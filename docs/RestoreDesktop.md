# docs/icon-layout.md

## 桌面图标布局管理 (RestoreDesktop)

### 概述

利用 Windows Shell COM 接口（`IFolderView2`）获取桌面图标的当前位置，并保存为 JSON 文件；之后可以从 JSON 文件中读取并批量恢复图标位置。

### 编译依赖

- Qt Widgets 模块
- 链接库：`ole32`, `shell32`, `uuid`, `oleaut32`, `shlwapi`, `comctl32`
- Windows SDK

### 使用方法

1. 运行 `RestoreDesktop.exe`
2. 点击 **保存当前布局...**  
   - 选择保存的 JSON 文件路径（如 `desktop_layout.json`）
   - 程序会遍历当前桌面上所有图标，记录其显示名称和坐标（`x, y`）
3. 点击 **恢复布局...**  
   - 选择之前保存的 JSON 文件
   - 程序会匹配当前桌面上同名的图标，并将其移动到保存的坐标位置

### JSON 格式示例

```json
{
  "icons": [
    {
      "name": "此电脑",
      "x": 100,
      "y": 120
    },
    {
      "name": "回收站",
      "x": 300,
      "y": 120
    }
  ]
}
```

### 注意事项

- 图标通过**显示名称**进行匹配，如果桌面上的文件/快捷方式被重命名，则恢复时会忽略该图标。
- 恢复位置时，会一次性移动所有匹配到的图标，桌面视图会自动刷新。
- 如果桌面图标数量变化（增加或删除），恢复时只会移动能匹配到的图标，其它图标位置不受影响。
- 需要以**普通用户权限**运行即可，不需要管理员权限。

### 技术细节

- 使用 `CoCreateInstance(CLSID_ShellWindows)` 获取 Shell 窗口对象。
- 通过 `FindWindowSW` 定位桌面窗口，获得 `IShellBrowser` → `IShellView` → `IFolderView2`。
- 遍历图标时调用 `GetItemPosition` 获取坐标，`GetDisplayNameOf` 获取名称。
- 恢复时使用 `SelectAndPositionItems` 批量移动。