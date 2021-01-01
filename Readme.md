# WebSocketAPI


## 玩家加入(服务端发出
## when a playe join the server
```json
{
    "type":"pack",
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
>    "cause": "decodefailed",//密匙不匹配无法解密
>    "params": {
>        "msg": "密匙不匹配，无法解密数据包！"
>    }
>}
>```
---
> - 发送全服消息(计划)
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
> - 发送个人消息(计划)
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
    "cause": "decodefailed",
    "params": {
        "msg": "格式错误:Unexpected character encountered while parsing value: d. Path '', line 0, position 0."
    }
}
```
- 请求无效（裸包试图执行搞权限操作）
```json
{
    "type": "pack",
    "cause": "invalidrequest",
    "params": {
        "msg": "未加密的初始包不予执行！"
    }
}
```
## Way to get the passwd
see passwdgetdemo.cpp