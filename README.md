# 权利保留
遵循**traffic server**的开源协议，欢迎大家转载、使用，请注明来源 https://github.com/BBD-RD/traffic-server-hosts/ 。

# 插件说明
欢迎大家转载、使用。转载、使用请注明来源 https://github.com/BBD-RD/traffic-server-hosts/ 。 也欢迎大家交流、讨论，联系方式见下方。

# 联系作者
liang.li@sinobbd.com 李亮

beixing.zuo@sinobbd.com 左北星

# 插件编译与安装
## 1. 下载代码
git clone https://github.com/BBD-RD/traffic-server-hosts/

## 2. 修改Makefile文件
```
#将下面的路径设置为你自己的traffic server源码路径
ATSSRCDIR=../../traffic-server
```

## 3. 增加新的api，用来获取**traffic server**回源时使用的域名
此步骤需要修改**traffic server**的源码，改动较小就不提供patch了。
* 首先是**proxy/api/ts/ts.h**文件
```
//添加如下函数声明
tsapi TSReturnCode TSAfterRemapHostGet(TSHttpTxn txnp, char *host, int len);
```
* 再是**proxy/InkAPI.cc**文件
```
//添加如下代码
TSReturnCode
TSAfterRemapHostGet(TSHttpTxn txnp, char *host, int len)
{
    int host_len = 0;
    HttpSM *sm = reinterpret_cast<HttpSM *>(txnp);

    HTTPHdr *incoming_request = &sm->t_state.hdr_info.client_request;

    const char *host_name = incoming_request->host_get(&host_len);
    if (len <= host_len) {
        return TS_ERROR;
    }

    strncpy(host, host_name, host_len);
    host[host_len] = 0;

    return TS_SUCCESS;
}
```

## 4. 编译并安装**traffic server**
```
#在traffic server源码目录执行
./configure
make && make install
```
此步骤可忽略，如果你已经做过。

## 5. 编译并安装**hosts**插件
```
#在hosts插件源码目录执行
make && make install
```

## 6. 插件配置
在**etc/trafficserver/plugin.config**中配置，如下
```
ws_hosts.so /path/to/hosts
```
可以配置绝对路径，也可以配置相对路径。如果不配置路径，默认使用**/etc/hosts**

## 7. 启动traffic server
不用写了把 ^_^

# 欢迎使用、转载、交流
