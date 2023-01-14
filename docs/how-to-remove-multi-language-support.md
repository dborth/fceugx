# 如何去掉对多语种的支持


主要需要修改四个地方：

## 1. 关闭代码中的多语种支持宏定义

确认项目 `source\fceugx.h` 文件中，第 83 行的宏定义代码已经被注释掉：

``` c++
// #define MULTI_LANGUAGE_SUPPORT
```

## 2. 在代码中定义要使用的语种

在代码中，使用什么语种由枚举常量 `LANG_DEFAULT` 的取值决定。中文版本使用的语种是简体中文，因此在项目 `source\fceugx.h` 文件的第 80 行，可以看到下面的代码：

``` c++
LANG_DEFAULT = LANG_SIMP_CHINESE
```

## 3. 确认默认字体文件与语种对应

- 默认字体文件，位于项目的 `source\fonts\font.ttf`。启动编译之后，编译器会根据这个路径找到 `font.ttf`，并把它作为内部资源，合并到最后编译生成的 `.dol` 文件中；

- 中文版本各个分支的默认语种都是简体中文，所以默认字体文件 `source\fonts\font.ttf` 使用 `fonts\zh.ttf` 进行了替换。

## 4. 不需要打包字体文件

在 `cn-only` 分支的 Build 脚本（`workflows\build.yml`）中，所有拷贝字体文件的操作都会被注释掉：

```
# cp fonts/en.ttf dist/FCEUltraGX/apps/fceugx/
# cp fonts/jp.ttf dist/FCEUltraGX/apps/fceugx/
# cp fonts/ko.ttf dist/FCEUltraGX/apps/fceugx/
# cp fonts/zh.ttf dist/FCEUltraGX/apps/fceugx/
```

## 5. 举一反三，以此类推

参考以上四步的做法，如果想构建一个日本语版本给你的老师用，相应的操作就是：

- 关闭宏定义
  ``` c++
  // #define MULTI_LANGUAGE_SUPPORT
  ```
- 定义默认语种
  ``` c++
  LANG_DEFAULT = LANG_JAPANESE
  ```
- 替换默认字体文件，也就是用 `fonts\jp.ttf` 替换 `source\fonts\font.ttf`
- 移除 Build 脚本中拷贝字体文件的操作
  ```
  # cp fonts/en.ttf dist/FCEUltraGX/apps/fceugx/
  # cp fonts/jp.ttf dist/FCEUltraGX/apps/fceugx/
  # cp fonts/ko.ttf dist/FCEUltraGX/apps/fceugx/
  # cp fonts/zh.ttf dist/FCEUltraGX/apps/fceugx/
```
