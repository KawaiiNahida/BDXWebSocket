# WebSocketAPI


## 玩家加入(服务端发出
## when a playe join the server
```jsonc
{
    "cause": "join",
    "params": {
        "sender": "WangYneos",
        "xuid": "",
        "ip": "target's ip address"
    }
}
```

## 玩家退出(服务端发出
## when the player left the server
```jsonc
{
    "cause": "left",
    "params": {
        "sender": "gxh2004",
        "xuid": "",
        "uuid": "",
        "ip": "target's ip address"
    }
}
```
## 玩家使用命令(服务端发出
## when the player use a command
```jsonc
{
    "cause": "cmd",
    "params": {
        "sender": "gxh2004",
        "cmd": "/kill @s"
    }
}
```
## 玩家消息(服务端发出
## player send a message
```jsonc
{
    "cause": "chat",
    "params": {
        "sender": "WangYneos",
        "text": "HelloWorld"
    }
}
```


## WS客户端控制命令
## WebSocket Client execute a command
> - 发送命令(不需要斜杠)
>```jsonc
>{
>    "is_encrypt":true,
>    "action": "runcmdrequest",
>    "params": {
>        "cmd": "kick WangYneos nmsl",
>        "id": 0
>    }
>}
>```
> - - 服务端返回
>```jsonc 
>{
>    "cause": "runcmdfeedback",
>    "params": {
>        "id": 0,
>        "result": ">命令执行结果<"
>    }
>}
>```
>```jsonc
>{
>    "cause": "authfailed",//密匙不匹配无法解密
>    "params": {
>        "msg": "密匙不匹配，无法解密数据包！"
>    }
>}
>```
---
> - 发送全服消息(计划)(client side)
>```jsonc
>{
>    "action": "broadcast",
>    "params": {
>        "text": "欢迎来到xxx"
>    }
>}
>```
---
> - 发送个人消息(计划)(client side)
>```jsonc
>{
>    "type":"pack",
>    "action": "tellraw",
>    "params": {
>        "target": "gxh2004",
>        "text": "欢迎来到xxx"
>    }
>}
>```

## 密文数据包
- 加密包(client)
```jsonc
{
    "type": "encrypted",

    "params": {
        "mode": "aes_cbc_pck7padding",
        "raw": "base64 String"
    }
}
```
- 解密错误
```jsonc
{
    "type": "pack",
    "cause": "decodefailed",
    "params": {
        "msg": "JSON格式错误、AES解密错误..."
    }
}
```
- 请求无效（裸包试图执行搞权限操作）

```jsonc
{
    "type": "pack",
    "cause": "error",
    "params": {
        "msg": "JsonParseError [type] Not Found or Not a string"
    }
}

```
