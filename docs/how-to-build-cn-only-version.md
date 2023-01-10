# 如何构建简体中文版本，并禁用多语种切换


主要需要修改四个地方：

## 1. 关闭代码中的多语种支持宏定义

确认以下位于项目 `source\fceugx.h` 文件第 83 行的代码已经被注释掉：

``` c++
// #define MULTI_LANGUAGE_SUPPORT
```

## 2. 在代码中定义默认语种

代码中的默认语种由枚举常量 `LANG_DEFAULT` 的取值决定。简体中文版本的默认语种自然就是简体中文，因此在项目 `source\fceugx.h` 文件的第 80 行，可以看到下面的代码：

``` c++
LANG_DEFAULT = LANG_SIMP_CHINESE
```

## 3. 确认默认字体文件与默认语种对应

- 默认字体文件，位于项目的 `source\fonts\font.ttf`。启动编译之后，编译器会根据这个路径找到 `font.ttf`，并把它作为内部资源，合并到最后编译生成的 `.dol` 文件中；

- `master` 分支上的默认字体文件对应的是英语，所以在创建好 `develop` 分支之后，原来的默认字体文件 `source\fonts\font.ttf` 被转存到了 `fonts\en.ttf`；

- `develop` 分支上的默认语种是简体中文，所以 `develop` 分支上的默认字体文件 `source\fonts\font.ttf` 已经被换成了 `fonts\zh.ttf`；

- `cn-only` 分支和 `develop` 分支的默认语种、默认字体都是简体中文。

## 4. 不需要打包字体文件

在 `cn-only` 分支的 Build 脚本（`workflows\build.yml`）中，所有拷贝字体文件的操作已经被注释掉：

```
# cp fonts/en.ttf dist/FCEUltraGX/apps/fceugx/
# cp fonts/jp.ttf dist/FCEUltraGX/apps/fceugx/
# cp fonts/ko.ttf dist/FCEUltraGX/apps/fceugx/
# cp fonts/zh.ttf dist/FCEUltraGX/apps/fceugx/
```

## 5. 举一反三，以此类推

参考以上四步构建简体中文版本的做法，如果想构建一个日本语版本给你的老师，并禁用多语种切换，主要的操作就是：

- 关闭宏定义
  ``` c++
  // #define MULTI_LANGUAGE_SUPPORT
  ```
- 定义默认语种
  ``` c++
  LANG_DEFAULT = LANG_JAPANESE
  ```
- 替换默认字体文件，用项目的 `fonts\jp.ttf` 替换 `source\fonts\font.ttf`
- 不需要拷贝字体文件
  ```
  # cp fonts/en.ttf dist/FCEUltraGX/apps/fceugx/
  # cp fonts/jp.ttf dist/FCEUltraGX/apps/fceugx/
  # cp fonts/ko.ttf dist/FCEUltraGX/apps/fceugx/
  # cp fonts/zh.ttf dist/FCEUltraGX/apps/fceugx/
```
