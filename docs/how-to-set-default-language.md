# 如何把默认语种设置成简体中文


主要需要修改四个地方：

## 1. 打开代码中的多语种支持宏定义

确认以下位于项目 `source\fceugx.h` 文件第 83 行的代码没有被注释掉：

``` c++
#define MULTI_LANGUAGE_SUPPORT
```

## 2. 在代码中定义默认语种

默认语种在代码中由枚举常量 `LANG_DEFAULT` 的取值决定。中文版的默认语种是简体中文，因此在项目 `source\fceugx.h` 文件的第 80 行，可以看到下面的代码：

``` c++
LANG_DEFAULT = LANG_SIMP_CHINESE
```

## 3. 确认默认字体文件与默认语种对应

- 默认字体文件，位于项目的 `source\fonts\font.ttf`。启动编译之后，编译器会根据这个路径找到 `font.ttf`，并把它作为内部资源，合并到最后编译生成的 `.dol` 文件中；

- `master` 分支上的默认字体文件对应的是英语，所以在创建好 `cn-full` 分支之后，原来的默认字体文件 `source\fonts\font.ttf` 被转存到了 `fonts\en.ttf`；

- `cn-full` 分支上的默认语种是简体中文，所以 `cn-full` 分支上的默认字体文件 `source\fonts\font.ttf` 已经被换成了 `fonts\zh.ttf`。

## 4. 打包非默认语种对应的字体文件

对于中文版来说，非默认语种对应的字体文件有三个，即 `en.ttf`、`jp.ttf` 和 `ko.ttf`，详情可以参考《[官方版本 vs. 中文版本](./what-i-did-in-fceugx-cn.md)》中 1.2 部分的表格。因此在 `cn-full` 分支的 Build 脚本（`workflows\build.yml`）中需要增加以下拷贝字体文件的操作：

```
cp fonts/en.ttf dist/FCEUltraGX/apps/fceugx/
cp fonts/jp.ttf dist/FCEUltraGX/apps/fceugx/
cp fonts/ko.ttf dist/FCEUltraGX/apps/fceugx/
# cp fonts/zh.ttf dist/FCEUltraGX/apps/fceugx/
```

最后一行被注释掉是因为简体中文是默认语种，已经通过默认字体文件合并到编译生成的 `.dol` 文件中，不需要再重复打包。

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
- 替换默认字体文件，用项目的 `fonts\jp.ttf` 替换 `source\fonts\font.ttf`
- 在 Build 脚本中拷贝字体文件
  ```
  cp fonts/en.ttf dist/FCEUltraGX/apps/fceugx/
  # cp fonts/jp.ttf dist/FCEUltraGX/apps/fceugx/
  cp fonts/ko.ttf dist/FCEUltraGX/apps/fceugx/
  cp fonts/zh.ttf dist/FCEUltraGX/apps/fceugx/
```
