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
>        "id": 0,
>        "token": "xxxxxxxxxx"
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
>        "success": true,
>        "result": ">命令执行结果<"
>    }
>}
>```
>```json
>{
>    "type":"pack",
>    "cause": "runcmdfeedback",
>    "params": {
>        "id": 0,
>        "success": false,
>        "result": "密匙不匹配，命令未执行！"
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
>        "text": "欢迎来到xxx",
>        "token": "xxxxxxxxxx"
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
>        "text": "欢迎来到xxx",
>        "token": "xxxxxxxxxx"
>    }
>}
>```

## 密文数据包
```json
{
    "type": "encrypted",
    "params": {
        "mode": "AES256",
        "raw": "xxxxxxxxx"
    }
}
```

## Way to get the passwd
see passwdgetdemo.cpp