# Allocate Bitmap In Native Heap

support Android 6.x - 7.x

# Details

参考 https://juejin.cn/post/7096059314233671694

但是原文有各种问题，比如 Android5.x 的释放逻辑并不如文中所说

此版本适配 Andorid 6.0 - 7.1

具体原理和原文大差不差，有一些细节问题

目前还在测试中，不确定是否存在稳定性问题，如要线上使用，需自测(顺便 star 一下呗)

# 

后测试发现，因为众多老机器使用的事 armeabi 架构，而不是 armeabi-v7a 或者 v8a，shadowhook 无法生效，也会有 crash

所以，可以考虑换一种 hook 方法。
