# WebSocketAPI


## 玩家加入(服务端发出
## when a playe join the server
```json
{
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
>    "action": "tellraw",
>    "params": {
>        "target": "gxh2004",
>        "text": "欢迎来到xxx",
>        "token": "xxxxxxxxxx"
>    }
>}
>```


## 密码获得规则
明文密码 + "@" + 不带token项的压缩JSON内容 --> AES256加密

## Way to get the passwd
see passwdgetdemo.cpp