# 如何把默认语种设置成简体中文


主要需要修改四个地方：

## 1. 打开代码中的多语种支持宏定义

确认项目 `source\fceugx.h` 文件中，第 83 行的宏定义代码没有被注释掉：

``` c++
#define MULTI_LANGUAGE_SUPPORT
```

## 2. 在代码中定义默认语种

在代码中，默认语种由枚举常量 `LANG_DEFAULT` 的取值决定。中文版本的默认语种是简体中文，因此在项目 `source\fceugx.h` 文件的第 80 行，可以看到下面的代码：

``` c++
LANG_DEFAULT = LANG_SIMP_CHINESE
```

## 3. 确认默认字体文件与默认语种对应

- 默认字体文件，位于项目的 `source\fonts\font.ttf`。启动编译之后，编译器会根据这个路径找到 `font.ttf`，并把它作为内部资源，合并到最后编译生成的 `.dol` 文件中；

- 中文版本各个分支的默认语种都是简体中文，所以默认字体文件 `source\fonts\font.ttf` 使用 `fonts\zh.ttf` 进行了替换；

- 官方版本使用的默认字体文件另存为 `fonts\en.ttf`。

## 4. 打包非默认语种对应的字体文件

对于中文版本来说，非默认语种对应的字体文件有三个，即 `en.ttf`、`jp.ttf` 和 `ko.ttf`，详情可以参考《[官方版本 vs. 中文版本](./what-i-did-in-fceugx-cn.md)》中 1.2 部分的表格。因此在 `master` 分支的 Build 脚本（`workflows\build.yml`）中，需要有以下拷贝字体文件的操作：

```
cp fonts/en.ttf dist/FCEUltraGX/apps/fceugx/
cp fonts/jp.ttf dist/FCEUltraGX/apps/fceugx/
cp fonts/ko.ttf dist/FCEUltraGX/apps/fceugx/
# cp fonts/zh.ttf dist/FCEUltraGX/apps/fceugx/
```

最后一行被注释掉是因为简体中文是默认语种，对应的默认字体文件在编译的时候，会合并到生成的 `.dol` 文件中，不需要再重复打包。

## 5. 举一反三，以此类推

参考以上四步的做法，如果想把默认语种设置成日本语，方便你的老师使用，相应的操作就是：

- 打开宏定义
  ``` c++
  #define MULTI_LANGUAGE_SUPPORT
  ```
- 定义默认语种
  ``` c++
  LANG_DEFAULT = LANG_JAPANESE
  ```
- 替换默认字体文件，也就是用 `fonts\jp.ttf` 替换 `source\fonts\font.ttf`
- 在 Build 脚本中拷贝除了 `jp.ttf` 以外的字体文件
  ```
  cp fonts/en.ttf dist/FCEUltraGX/apps/fceugx/
  # cp fonts/jp.ttf dist/FCEUltraGX/apps/fceugx/
  cp fonts/ko.ttf dist/FCEUltraGX/apps/fceugx/
  cp fonts/zh.ttf dist/FCEUltraGX/apps/fceugx/
```
