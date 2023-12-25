# webMD

webMD 是一个简单的Markdown服务器，它可以将Markdown文件渲染为HTML并通过Web服务器提供服务。

## 特点

- 轻量级Markdown服务器
- 支持Markdown文件实时渲染
- 简单易用的Web界面

## 开始使用

以下指南将帮助您在本地机器上安装和运行webMD，用于开发和测试目的。

### 先决条件

在开始之前，确保您的系统已安装以下软件：

- GCC编译器
- Make工具
- cmark库

### 安装

1. 克隆仓库到本地机器：

   ```bash
   git clone https://github.com/TonWin618/webMD.git

1. 进入项目目录：

   ```
   cd webMD
   ```

2. 编译项目：

   ```
   make
   ```

### 运行

运行编译后的程序：

```
./bin/webMD
```

默认情况下，服务器将在主机中任意一个可用的端口上运行。您可以通过在命令行中指定端口号来更改端口。

```
./bin/webMD 8080
```

## 使用

在浏览器中访问 `http://localhost:8080`（或您指定的端口）以查看Markdown文件。
请将所有Markdown文件放置在bin/mds目录下，以便程序能够正确获取。

## 版权和许可

此项目根据MIT许可证发布。有关详细信息，请参阅`LICENSE`文件。

## 联系方式

- 作者：TonWin
- 邮箱：tonwin618@qq.com

## 致谢

- 感谢所有为此项目做出贡献的开发者。
- 特别感谢 cmark 库的开发者们，该库在本项目中用于Markdown文件的解析和渲染。
- 对 tinyhttpd 项目表示感谢，本项目在实现Web服务器功能时参考了它的一部分代码。