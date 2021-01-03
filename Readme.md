# WebSocketAPI


## 玩家加入(服务端发出
## when a playe join the server
```json
{
    "type":"pack"
    "cause": "join",
    "params": {
        "sender": "WangYneos",
        "xuid": "",
        "uuid": "",
        "ip": "target's ip address"
    }
}
```

## 玩家退出(服务端发出
## when the player left the server
```json
{
    "type":"pack",
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
```json
{
    "type":"pack",
    "cause": "cmd",
    "params": {
        "sender": "gxh2004",
        "cmd": "/kill @s"
    }
}
```
## 玩家消息(服务端发出
## player send a message
```json
{
    "type":"pack",
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
>```json
>{
>    "type":"pack",
>    "action": "runcmdrequest",
>    "params": {
>        "cmd": "kick WangYneos nmsl",
>        "id": 0
>    }
>}
>```
> - - 服务端返回
>```json 
>{
>    "type":"pack",
>    "cause": "runcmdfeedback",
>    "params": {
>        "id": 0,
>        "result": ">命令执行结果<"
>    }
>}
>```
>```json
>{
>    "type":"pack",
>    "cause": "authfailed",//密匙不匹配无法解密
>    "params": {
>        "msg": "密匙不匹配，无法解密数据包！"
>    }
>}
>```
---
> - 发送全服消息(计划)(client side)
>```json
>{
>    "type":"pack",
>    "action": "broadcast",
>    "params": {
>        "text": "欢迎来到xxx"
>    }
>}
>```
---
> - 发送个人消息(计划)(client side)
>```json
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
- 加密包
```json
{
    "type": "encrypted",
    "params": {
        "mode": "AES256",
        "raw": "xxxxxxxxx"
    }
}
```
- 解密错误
```json
{
    "type": "pack",
    "cause": "authfailed",
    "params": {
        "msg": "格式错误:Unexpected character encountered while parsing value: d. Path '', line 0, position 0."
    }
}
```
- 请求无效（解析包出错）
```json
{
    "type": "pack",
    "cause": "error",
    "params": {
        "msg": "JsonParseError [type] Not Found or Not a string"
    }
}
```
##Uses AES256/ECB/PCK5Padding
