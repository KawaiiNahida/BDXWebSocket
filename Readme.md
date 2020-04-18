# WebSocketAPI

## 玩家消息(服务端发出
## player send a message
```json
{"operate":"onmsg","target":"WangYneos","text":"HelloWorld"}
//操作标识——————————目标——————————————————返回信息（玩家聊天内容）
```

## 玩家加入(服务端发出
## when a playe join the server
```json
{"operate":"onjoin","target":"WangYneos","text":"target's ip address"}
//操作标识——————————---目标——————————————————返回信息（玩家ip）
```

## 玩家退出(服务端发出
## when the player left the server
```json
{"operate":"onleft","target":"WangYneos","text":"Lefted server"}
//与上面类似
```

## 玩家使用命令(服务端发出
## when the player use a command
```json
{"operate":"onCMD","target":"WangYneos","text":"/list"}
//操作标识-----------目标玩家--------------执行的命令
```

## WS客户端使用命令
## WebSocket Client execute a command
>发送
>send
```json
{"operate":"runcmd","passwd":"CD92DDCEBFB8D3FB1913073783FAC0A1","cmd":"in_game command here"}
//标识--操作类型--密码---------------------------------------执行内容----------------
```
>服务端返回
>feedback by server
```json
{"operate":"runcmd","Auth":"PasswdMatch","text":"Command Feedback"}
//操作标识---操作类型--密码验证--成功---------返回内容----------------------------
{"operate":"runcmd","Auth":"Failed”,"text":"Password Not Match" }
//操作标识---操作类型--出错-------验证---------返回内容--------------
```

## 密码获得规则
服务端获取密码

+当前 年月日时分

（无分号，空格

例如密码是passwd

则验证密码passwd202004062016

（2020年4月6日8点16

取MD5（大写

得到CD92DDCEBFB8D3FB1913073783FAC0A1

客户端与服务端一致则验证成功


## Way to get the passwd
1,get the base password(for example:passwd)

2,add (int)year month day hour minute(e.g :passwd2020040110)

3,get the MD5 vaule and it's the passwd

(one passwd can be use in 2 min