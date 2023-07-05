# Allocate Bitmap In Native Heap

support Android 6.x - 7.x

# 20230725 更新

因为使用了 shadowhook 排除了 armeabi 的类型，只处理 v7a v8a，崩溃量级很低(低端机也不多)，只有个位数

但是同时，低端机有一部分很古老的就是只支持 armeabi，所以并没能全覆盖

如果，想完全覆盖(其实没必要)，就不能用 shadowhook，需要自己去寻找函数符号，替换函数指针了，并且要使用老版本的 ndk 编译

# Details

参考 https://juejin.cn/post/7096059314233671694

但是原文有各种问题，比如 Android5.x 的释放逻辑并不如文中所说

此版本适配 Andorid 6.0 - 7.1

具体原理和原文大差不差，有一些细节问题

目前还在测试中，不确定是否存在稳定性问题，如要线上使用，需自测(顺便 star 一下呗)

注意 `shadowhook 只支持 armeabi-v7a arm64-v8a`
