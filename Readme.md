# WebSocketAPI

## 玩家消息(服务端发出
```json
{"operate":"onmsg","target":"WangYneos","text":"HelloWorld"}
//操作标识——————————目标——————————————————返回信息（玩家聊天内容）
```

## 玩家加入(服务端发出
```json
{"operate":"onjoin","target":"WangYneos","text":"Joined server"}
//操作标识——————————---目标——————————————————返回信息（加入了服务器）
```

## 玩家退出(服务端发出
```json
{"operate":"onleft","target":"WangYneos","text":"Lefted server"}
//与上面类似
```

## 玩家使用命令(服务端发出
```json
{"operate":"onCMD","target":"WangYneos","CMD":"/list"}
//操作标识-----------目标玩家--------------执行的命令
```

## WS客户端使用命令
>发送
```json
{"op":"runcmd","passwd":"CD92DDCEBFB8D3FB1913073783FAC0A1","cmd":"kick WangYneos"}
//标识--操作类型--密码---------------------------------------执行内容----------------
```
>服务端返回
```json
{"operate":"runcmd","Auth":"PasswdMatch","feedback":"commands.kick.success"}
//操作标识---操作类型--密码验证--成功---------返回内容----------------------------
{"operate":"runcmd","onError":"Auth","text":"Password Not Match"}
//操作标识---操作类型--出错-------验证---------返回内容--------------
{"operate":"onCMD","target":"gxh2004","CMD":"/kick gxh2004"}
```

## 密码获得规则
>服务端获取密码

>+当前 年月日时分

>（无分号，空格

>例如密码是passwd

>则验证密码passwd202004062016

>（2020年4月6日8点16

>取MD5（大写

>得到CD92DDCEBFB8D3FB1913073783FAC0A1

>客户端与服务端一致则验证成功