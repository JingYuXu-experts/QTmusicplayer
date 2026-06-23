# QTmusicplayer

QTmusicplayer 是一个使用 Qt 6、QML 和 C++ 开发的桌面音乐播放器，集成本地音乐库、搜索、歌词、MV、播放队列、公共聊天室以及 Dify AI 音乐助手。

项目采用可移植目录设计：音乐数据库、媒体文件和 Dify 配置均使用项目内相对路径。将仓库克隆到其他电脑后，只需安装 Git LFS 和对应的 Qt 开发环境即可重新构建，不需要修改盘符或本机绝对路径。

仓库地址：[JingYuXu-experts/QTmusicplayer](https://github.com/JingYuXu-experts/QTmusicplayer)

## 功能概览

| 功能 | 说明 |
| --- | --- |
| 本地音乐播放 | 支持播放、暂停、上一首、下一首、进度跳转和音量调节 |
| 播放模式 | 支持列表循环、单曲循环和随机播放 |
| 首页与搜索 | 浏览本地歌曲，并按歌曲名或歌手搜索 |
| 媒体库 | 使用项目内 SQLite 数据库管理歌曲信息 |
| 歌词 | 读取 `.lrc` 或 `.txt` 文件并跟随播放进度显示 |
| 本地 MV | 播放歌曲对应的本地视频，进入 MV 前会停止音频播放 |
| 播放队列 | 查看、切换和移除待播放歌曲 |
| Music Chat | 程序内置本地 TCP 聊天服务和模拟好友消息 |
| Dify AI 助手 | 调用已发布的 Dify 对话应用，提供音乐推荐、歌曲问答和播放器使用帮助 |
| 跨电脑迁移 | 数据库和媒体文件均使用相对路径，不依赖原电脑盘符 |

## 技术栈

- C++17
- Qt 6.4 或更高版本
- Qt Quick / QML
- Qt Quick Controls 2
- Qt Multimedia
- Qt Network
- Qt SQL / SQLite
- Qt SVG
- CMake 3.16 或更高版本
- Git LFS
- Dify Chat Messages API

本项目已在 Windows、Qt 6.10.2、MinGW 13.1 和 Ninja 环境下完成全新构建及启动验证。

## 项目目录

```text
QTmusicplayer/
├─ AppleMusic/
│  ├─ CMakeLists.txt
│  ├─ main.cpp
│  ├─ MusicManager.cpp/.h       # 音乐库、播放器、歌词与媒体路径
│  ├─ DifyClient.cpp/.h         # Dify API 网络客户端
│  ├─ DifyView.qml              # AI 助手界面
│  ├─ ChatView.qml              # 本地公共聊天室界面
│  ├─ Main.qml                  # 主窗口和页面栈
│  ├─ Sidebar.qml               # 页面导航
│  ├─ PlayerBar.qml             # 底部播放控制栏
│  ├─ config/
│  │  └─ dify.ini               # 可移植 Dify 配置
│  ├─ data/
│  │  └─ music_library.db       # SQLite 音乐数据库
│  ├─ media/                    # 音频、封面、歌词和 MV
│  └─ default_cover.svg
├─ .gitattributes               # Git LFS 规则
├─ .gitignore
└─ README.md
```

## 环境要求

目标电脑需要安装：

1. [Git](https://git-scm.com/)
2. [Git LFS](https://git-lfs.com/)
3. [Qt](https://www.qt.io/download-qt-installer) 6.4 或更高版本
4. CMake 3.16 或更高版本
5. 与 Qt Kit 匹配的编译器，例如 MinGW、LLVM-MinGW 或 MSVC

安装 Qt 时至少选择以下组件：

- Qt Quick
- Qt Quick Controls 2
- Qt Multimedia
- Qt Network
- Qt SQL
- Qt SVG

Qt Creator 通常会随 Qt 在线安装器一起安装，并自动配置 CMake、Ninja 和编译器。

## 克隆项目

先确保 Git LFS 已安装并初始化：

```powershell
git lfs install
```

然后克隆仓库并下载媒体文件：

```powershell
git clone https://github.com/JingYuXu-experts/QTmusicplayer.git
cd QTmusicplayer
git lfs pull
```

可以使用以下命令确认 LFS 文件已正确下载：

```powershell
git lfs status
```

如果 `.mp3`、`.mp4` 或 `.png` 文件只有几行文本，说明下载到的是 LFS 指针，需要重新执行 `git lfs pull`。

## 使用 Qt Creator 构建

1. 启动 Qt Creator。
2. 打开 `AppleMusic/CMakeLists.txt`。
3. 选择 Qt 6.4 或更高版本的 Desktop Kit。
4. 点击“Configure Project”。
5. 选择 Debug 或 Release 配置并构建。
6. 运行 `AppleMusic`。

CMake 会在每次链接程序后自动将：

```text
AppleMusic/config/dify.ini
```

复制到：

```text
<构建目录>/config/dify.ini
```

因此从 GitHub 克隆后，不需要再手工复制或创建 AI 配置。

## 使用命令行构建

Qt Creator 是推荐方式。如果 Qt、CMake 和 Ninja 已加入环境变量，也可以使用命令行：

```powershell
cd QTmusicplayer/AppleMusic
cmake -S . -B build/release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="你的Qt安装目录/6.x.x/mingw_64"
cmake --build build/release --parallel
```

运行：

```powershell
./build/release/AppleMusic.exe
```

安装到独立目录：

```powershell
cmake --install build/release --prefix build/package
```

安装步骤会同时复制 `AppleMusic.exe` 和 `config/dify.ini`。

## 本地音乐库

项目默认使用以下可移植目录：

- 数据库：`AppleMusic/data/music_library.db`
- 媒体文件：`AppleMusic/media/`
- 默认封面：`AppleMusic/default_cover.svg`

数据库只保存类似下面的相对路径：

```text
media/Anti-Hero.mp3
media/Anti-hero.png
media/Anti-Hero.lrc
media/我是如此相信.mp4
```

程序启动时会从当前项目目录或可执行文件附近寻找 `media` 和 `data`，再通过 Qt 路径 API 转换为实际文件地址。因此项目可以放在不同盘符、中文目录或带空格的目录中。

### 添加歌曲

将新文件放入 `AppleMusic/media/`，建议保持相同的基础文件名：

```text
Song Name.mp3
Song Name.png
Song Name.lrc
Song Name.mp4
```

也支持使用 `.txt` 保存歌词。程序启动时会：

1. 在数据库不存在时自动创建数据库和表。
2. 扫描 `media/` 中尚未入库的 `.mp3` 文件。
3. 自动关联同名封面、歌词和 MV。
4. 修复媒体文件移动后失效的相对路径。

## Dify AI 助手

点击侧边栏的“AI 助手”进入 Dify 问答页面。当前版本支持：

- 音乐和歌手相关问答
- 按风格、语言、年代或氛围推荐歌曲
- 播放器功能使用帮助
- 多行长文本显示
- 请求中状态提示
- 清空当前会话
- 网络错误、认证失败、限流、超时和空输出提示

AI 客户端与音乐播放器、本地数据库和 Music Chat 相互隔离。Dify 请求失败时，只会在 AI 页面显示错误，不会停止音乐或影响其他页面。

### API 调用

当前 Dify 应用是对话型应用，界面中显示为音乐对话引擎。程序调用：

```text
POST https://api.dify.ai/v1/chat-messages
```

请求使用阻塞模式。用户问题通过顶层 `query` 发送，可选的播放器状态上下文通过 `inputs.player_context` 发送：

```text
query=用户输入的问题
inputs.player_context=播放器上下文（可选）
```

Dify 返回的 `answer` 会显示在聊天气泡中。客户端仍保留 Workflow 兼容逻辑；如果以后切回 Workflow，可在配置中把 `appMode` 改为 `workflow` 并设置对应的 `inputKey`。

### Dify 配置

默认配置文件：

```text
AppleMusic/config/dify.ini
```

配置结构：

```ini
[Dify]
apiBaseUrl=https://api.dify.ai/v1
apiKey=你的Dify应用Key
appMode=chat
contextKey=player_context
timeoutMs=95000
```

程序会按顺序在可执行文件附近和项目目录中寻找配置。也可以通过环境变量覆盖：

- `DIFY_CONFIG_FILE`：指定其他配置文件
- `DIFY_API_BASE_URL`：覆盖 API 地址
- `DIFY_API_KEY`：覆盖认证 Key

仓库当前按“克隆后无需配置即可运行”的要求提交了应用 Key。任何能访问仓库的人都能读取并使用该 Key。如果发生泄露、滥用、额度耗尽或 Key 被撤销，需要在 Dify 后台重新生成，并更新 `AppleMusic/config/dify.ini`。

## Music Chat

程序启动时会在本机 `12345` 端口启动聊天服务，ChatView 通过 `127.0.0.1` 连接。该功能仅用于当前电脑上的演示，不是互联网聊天室。

如果端口已被其他程序占用，音乐播放和 Dify AI 功能仍可正常使用，但 Music Chat 可能显示连接失败。

## Git LFS

以下大文件类型通过 Git LFS 管理：

- `*.mp3`
- `*.mp4`
- `*.png`
- `*.rar`

新增媒体文件后建议检查：

```powershell
git check-attr filter -- AppleMusic/media/你的文件.mp3
```

正确结果应包含：

```text
filter: lfs
```

## 常见问题

### CMake 找不到 Qt6

确认选择了有效的 Qt Desktop Kit，或在命令行配置时设置正确的 `CMAKE_PREFIX_PATH`。

### 程序能启动但歌曲无法播放

执行：

```powershell
git lfs pull
```

并确认音频文件位于 `AppleMusic/media/`，不是 Git LFS 指针文本。

### 图片、歌词或 MV 不显示

确认相关文件与歌曲使用相同的基础文件名，并位于 `AppleMusic/media/`。

### AI 页面显示配置不可用

确认以下任一位置存在 `config/dify.ini`：

- 源码目录 `AppleMusic/config/dify.ini`
- 可执行文件所在目录下的 `config/dify.ini`

重新构建程序会自动复制配置。

### Dify 返回 400

确认当前应用类型和配置一致。对话应用应使用 `appMode=chat`，并通过 `/chat-messages` 发送顶层 `query`；如果切回 Workflow，才需要设置 `appMode=workflow` 和对应的 `inputKey`。

### Dify 返回 401 或 403

API Key 已失效或不属于当前发布的应用，需要重新生成并更新配置。

### Dify 请求超时

当前超时时间默认为 95 秒。可以修改 `timeoutMs`，但 Dify Cloud 的阻塞请求还受到服务端超时限制。

### Music Chat 无法连接

检查本机 `12345` 端口是否被占用。该问题不会影响播放器和 AI 助手。

## 开发与验证

修改代码后建议至少执行：

```powershell
cmake -S AppleMusic -B AppleMusic/build/test
cmake --build AppleMusic/build/test --parallel
```

发布前应验证：

- 首页、搜索和媒体库正常显示
- 播放、暂停、切歌、进度和音量正常
- 歌词与 MV 正常打开
- Music Chat 能连接本地服务
- AI 助手能返回 Dify 对话应用输出
- 断网或错误 Key 不会导致程序崩溃
- 全新构建目录会自动生成 `config/dify.ini`

构建输出和 Qt Creator 用户配置已通过 `.gitignore` 排除，不应提交到仓库。

## License

本项目使用仓库中的 [LICENSE](LICENSE) 文件所声明的许可证。
