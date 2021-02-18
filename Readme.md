# WebSocketAPI


## 玩家加入(服务端发出
## when a playe join the server
```jsonc
{
    "type":"pack",
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
    "type":"pack",
    "cause": "left",
    "params": {
        "sender": "gxh2004",
        "xuid": ""
    }
}
```
## 玩家使用命令(服务端发出
## when the player use a command
```jsonc
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
```jsonc
{
    "type":"pack",
    "cause": "die",
    "params": {
        "target": "WangYneos",
        "source": "gxh2004",
		"causecode": "114514(Not real)",
		"cause_name": "Killed By Other Player"
    }
}
```
## 生物死亡(服务端发出(仅限有名字（NameTag）的生物
## Mob Die
```json
{
  "type": "pack",
  "cause": "mobdie",
  "params": {
    "mobtype": "entity.player.name",//实体类型
    "mobname": "gxh2004",//实体名称
    "dmcase": 0,//伤害源ID
    "dmname": "",//伤害类型
    "srctype": "",//伤害源类型
    "srcname": "",//伤害源名称
    "pos": {
      "x": 1072.10034,
      "y": 69.62001,
      "z": 14.9382544
    }
  }
}
```


## WS客户端控制命令
## WebSocket Client execute a command
> - 发送命令(不需要斜杠)
>```jsonc
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
>```jsonc 
>{
>    "type":"pack",
>    "cause": "runcmdfeedback",
>    "params": {
>        "id": 0,
>        "result": ">命令执行结果<"
>    }
>}
>```
>```jsonc
>{
>    "type":"pack",
>    "cause": "decodefailed",//密匙不匹配无法解密
>    "params": {
>        "msg": "密匙不匹配，无法解密数据包！"
>    }
>}
>```
---
## 密文数据包
- 加密包
```jsonc
{
    "type": "encrypted",
    "params": {
        "mode": "aes_cbc_pck7padding",
        "raw": "base64 String(from a pack json)"
    }
}
```
- 解密错误
```jsonc
{
    "type": "pack",
    "cause": "decodefailed",
    "params": {
        "msg": "JSON格式错误、AES解密错误... 此处返回消息不固定，只用于描述"
    }
}
```
- 请求无效（插件端要求使用加密数据包）
```jsonc
{
    "type": "pack",
    "cause": "invalidrequest",
    "params": {
        "msg": "未加密的初始包不予执行！此处返回消息不固定，只用于描述"
    }
}
```
## 加密规则
-取密码大写MD5作为基密码  
-前16位作为AES_Key  
-后16位作为AES_IV  
-使用AES/CBC/PCKS7Padding(PCKS5Padding理论上通用)  
-见en_decrypt_bdxws_demo.java为例子
