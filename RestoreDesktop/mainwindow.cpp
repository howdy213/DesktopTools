#include "mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include <exdisp.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <windows.h>

#include "comptr.h"

// 从桌面 IShellWindows 获取 IFolderView2 接口
static ComPtr<IFolderView2> FindDesktopFolderView() {
    // 1. 获取 ShellWindows 对象
    ComPtr<IShellWindows> spShellWindows;
    HRESULT hr = CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&spShellWindows));
    if (FAILED(hr))
        return nullptr;

    // 2. 查找桌面窗口
    VARIANT vtLoc;
    vtLoc.vt = VT_I4;
    vtLoc.lVal = CSIDL_DESKTOP;
    VARIANT vtEmpty;
    vtEmpty.vt = VT_EMPTY;
    long hwnd = 0;
    ComPtr<IDispatch> spDisp;
    hr = spShellWindows->FindWindowSW(&vtLoc, &vtEmpty, SWC_DESKTOP, &hwnd,
                                      SWFO_NEEDDISPATCH, &spDisp);
    if (FAILED(hr))
        return nullptr;

    // 3. 从 Dispatch 获得 IShellBrowser
    ComPtr<IServiceProvider> spProvider;
    hr = spDisp->QueryInterface(IID_PPV_ARGS(&spProvider));
    if (FAILED(hr))
        return nullptr;

    ComPtr<IShellBrowser> spBrowser;
    hr = spProvider->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&spBrowser));
    if (FAILED(hr))
        return nullptr;

    // 4. 获得活动的 IShellView
    ComPtr<IShellView> spView;
    hr = spBrowser->QueryActiveShellView(&spView);
    if (FAILED(hr))
        return nullptr;

    // 5. 查询 IFolderView2
    ComPtr<IFolderView2> spFolderView;
    hr = spView->QueryInterface(IID_PPV_ARGS(&spFolderView));
    if (FAILED(hr))
        return nullptr;

    return spFolderView;
}

// 序列化：保存图标位置到 JSON 文件
static bool SaveIconPositionsToJson(const QString &filePath) {
    ComPtr<IFolderView2> pView = FindDesktopFolderView();
    if (!pView) {
        QMessageBox::warning(nullptr, "错误", "无法获取桌面视图。");
        return false;
    }

    // 获取图标数量（注意：使用 ItemCount）
    int itemCount = 0;
    HRESULT hr = pView->ItemCount(SVGIO_ALLVIEW, &itemCount);
    if (FAILED(hr) || itemCount == 0) {
        QMessageBox::information(nullptr, "提示", "桌面没有图标。");
        return false;
    }

    // 获取 IShellFolder 以显示名称
    ComPtr<IShellFolder> pFolder;
    hr = pView->GetFolder(IID_PPV_ARGS(&pFolder));
    if (FAILED(hr))
        return false;

    QJsonArray iconsArray;

    for (int i = 0; i < itemCount; ++i) {
        PITEMID_CHILD pidl = nullptr;
        if (FAILED(pView->Item(i, &pidl)) || !pidl)
            continue;

        POINT pt = {0, 0};
        hr = pView->GetItemPosition(pidl, &pt);
        if (FAILED(hr)) {
            CoTaskMemFree(pidl);
            continue;
        }

        // 获取图标显示名称
        STRRET str;
        hr = pFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &str);
        if (FAILED(hr)) {
            CoTaskMemFree(pidl);
            continue;
        }

        wchar_t szName[MAX_PATH] = {0};
        StrRetToBufW(&str, pidl, szName, MAX_PATH);
        QString name = QString::fromWCharArray(szName);

        QJsonObject iconObj;
        iconObj["name"] = name;
        iconObj["x"] = static_cast<int>(pt.x); // 显式转换为 int
        iconObj["y"] = static_cast<int>(pt.y);
        iconsArray.append(iconObj);

        CoTaskMemFree(pidl);
    }

    // 写入 JSON 文件
    QJsonObject root;
    root["icons"] = iconsArray;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(nullptr, "错误", "无法打开文件进行写入。");
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

// 反序列化：从 JSON 文件恢复图标位置
static bool RestoreIconPositionsFromJson(const QString &filePath) {
    // 读取 JSON
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(nullptr, "错误", "无法打开文件。");
        return false;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject() || !doc.object().contains("icons")) {
        QMessageBox::warning(nullptr, "错误", "无效的布局文件格式。");
        return false;
    }

    QJsonArray iconsArray = doc.object()["icons"].toArray();
    if (iconsArray.isEmpty()) {
        QMessageBox::information(nullptr, "提示", "布局文件中没有图标信息。");
        return false;
    }

    // 获取当前桌面视图
    ComPtr<IFolderView2> pView = FindDesktopFolderView();
    if (!pView) {
        QMessageBox::warning(nullptr, "错误", "无法获取桌面视图。");
        return false;
    }

    int currentCount = 0;
    pView->ItemCount(SVGIO_ALLVIEW, &currentCount);
    if (currentCount == 0)
        return false;

    ComPtr<IShellFolder> pFolder;
    if (FAILED(pView->GetFolder(IID_PPV_ARGS(&pFolder))))
        return false;

    // 先构建 JSON 中保存的名称 -> 目标坐标的映射
    QMap<QString, QPoint> targetMap;
    for (const QJsonValue &val : iconsArray) {
        QJsonObject obj = val.toObject();
        QString name = obj["name"].toString();
        int x = obj["x"].toInt();
        int y = obj["y"].toInt();
        targetMap.insert(name, QPoint(x, y));
    }

    // 遍历当前桌面图标，匹配并构造移动列表
    // 注意：存储的 PIDL 指针为 PCITEMID_CHILD (const)，但我们需要传递 const
    // 数组给 SelectAndPositionItems
    std::vector<PCITEMID_CHILD> pidlList;
    std::vector<POINT> ptList;

    for (int i = 0; i < currentCount; ++i) {
        PITEMID_CHILD pidl = nullptr;
        if (FAILED(pView->Item(i, &pidl)) || !pidl)
            continue;

        STRRET str;
        if (FAILED(pFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &str))) {
            CoTaskMemFree(pidl);
            continue;
        }

        wchar_t szName[MAX_PATH] = {0};
        StrRetToBufW(&str, pidl, szName, MAX_PATH);
        QString currentName = QString::fromWCharArray(szName);

        if (targetMap.contains(currentName)) {
            QPoint pt = targetMap[currentName];
            POINT p = {pt.x(), pt.y()};
            pidlList.push_back(pidl); // 直接使用 Item 返回的 PIDL（稍后统一释放）
            ptList.push_back(p);
        } else {
            CoTaskMemFree(pidl);
        }
    }

    if (pidlList.empty()) {
        QMessageBox::information(nullptr, "提示",
                                 "没有找到匹配的图标，请确认桌面内容未变化。");
        return false;
    }

    // 批量移动图标（MinGW 头文件要求第四个参数 DWORD，通常传 0）
    HRESULT hr = pView->SelectAndPositionItems(static_cast<UINT>(pidlList.size()),
                                               pidlList.data(), ptList.data(),
                                               0 // 第四个参数保留，必须为 0
                                               );

    // 释放所有 PIDL（注意去除 const 限定）
    for (auto p : pidlList)
        CoTaskMemFree((LPVOID)(p));

    if (FAILED(hr)) {
        QMessageBox::warning(nullptr, "错误", "移动图标失败。");
        return false;
    }

    return true;
}

// ---------- MainWindow 实现 ----------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("桌面图标布局管理");
    resize(300, 150);

    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    m_saveBtn = new QPushButton("保存当前布局...");
    m_restoreBtn = new QPushButton("恢复布局...");

    layout->addWidget(m_saveBtn);
    layout->addWidget(m_restoreBtn);
    setCentralWidget(centralWidget);

    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::saveLayout);
    connect(m_restoreBtn, &QPushButton::clicked, this,
            &MainWindow::restoreLayout);
}

void MainWindow::saveLayout() {
    QString filePath = QFileDialog::getSaveFileName(
        this, "选择保存位置", QString(), "JSON 文件 (*.json)");
    if (filePath.isEmpty())
        return;

    if (SaveIconPositionsToJson(filePath)) {
        QMessageBox::information(this, "成功", "桌面布局已保存。");
    }
}

void MainWindow::restoreLayout() {
    QString filePath = QFileDialog::getOpenFileName(
        this, "选择布局文件", QString(), "JSON 文件 (*.json)");
    if (filePath.isEmpty())
        return;

    if (RestoreIconPositionsFromJson(filePath)) {
        QMessageBox::information(this, "成功", "桌面布局已恢复。");
    }
}