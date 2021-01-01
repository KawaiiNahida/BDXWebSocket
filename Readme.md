# WebSocketAPI

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

## WS客户端控制命令
## WebSocket Client execute a command
>发送
>send
```json
{
    "action": "runcmdrequest",
    "params": {
        "cmd": "kick WangYneos nmsl",
        "id": 0,
        "token": "xxxxxxxxxx"
    }
}
```
>服务端返回
>
>feedback by server
```json 
{
    "cause": "runcmdfeedback",
    "params": {
        "id": 0,
        "success": true,
        "addition": ">命令执行结果<"
    }
}
```
```json
{
    "cause": "runcmdfeedback",
    "params": {
        "id": 0,
        "success": false,
        "addition": "密匙不匹配，命令未执行！"
    }
}
```

## 密码获得规则
见passwdgetdemo.cpp

## Way to get the passwd
see passwdgetdemo.cpp