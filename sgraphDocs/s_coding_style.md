# smartGraphics 代码风格

## 命名

- 自研目录使用 `sgraph` 前缀，例如 `sgraphCore`。
- 自研文件使用 `s_` 加小写下划线，例如 `s_cad_document.cpp`。
- 类、结构体、枚举类型和类型别名使用大写 `S` 前缀，例如 `SCadDocument`。
- 接口类型使用 `SI` 前缀，例如 `SIFileCodec`。
- 函数使用小驼峰，例如 `openDocument()`。
- 局部变量和参数使用小写下划线，例如 `entity_id`。
- 私有成员使用 `m_` 加小写下划线，例如 `m_entity_count`。
- 常量使用 `kPascalCase`；宏使用 `SMARTCAD_UPPER_SNAKE_CASE`。

## C++ 与 Qt

- 使用 C++17、4 空格、UTF-8、LF，行宽 100。
- 控制语句、函数、类型和命名空间使用 Allman 大括号风格，左大括号必须另起一行；
  禁止 `if (...) {`、`for (...) {` 等行尾左大括号写法。
- 每个自研 `.h`、`.cpp` 文件不得超过 800 个物理行（包含空行和注释）。接近
  700 行时应按职责提前拆分，禁止通过压缩格式、合并语句或删除必要注释规避限制。
- 拆分实现文件时保持类的公共接口稳定，并在所属模块的 `CMakeLists.txt` 中显式登记
  新文件。第三方源码保持上游原貌，不为满足本限制直接修改。
- 单参数构造函数必须 `explicit`，重写函数必须 `override`。
- 禁止裸拥有指针；非 QObject 默认使用 `std::unique_ptr`。
- QObject 使用父子所有权，弱引用使用 `QPointer`。
- 信号槽使用函数指针语法，禁止 `SIGNAL`/`SLOT` 字符串语法。
- 头文件禁止 `using namespace`，并且必须能够独立编译。
- 第三方 SDK 类型只能存在于适配层，不得泄漏到公共 CAD API。
- 公共 API 不传播异常，失败结果通过 `SResult<T>` 返回。
- 注释解释原因和约束，不复述代码。

## 提交门槛

- 通过 `.clang-format`。
- 通过 `/W4 /permissive- /Zc:__cplusplus /utf-8` 编译。
- 通过单元测试和 Debug/Release 构建。
- 不提交构建产物、第三方 SDK 压缩包、无授权字体和图纸。
