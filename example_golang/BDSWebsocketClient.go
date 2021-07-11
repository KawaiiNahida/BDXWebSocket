package main

import (
	"bufio"
	"bytes"
	"crypto/aes"
	"crypto/cipher"
	"crypto/md5"
	"encoding/base64"
	"fmt"
	"log"
	"math/rand"
	"net/url"
	"os"
	"strings"

	"github.com/gorilla/websocket"
	jsoniter "github.com/json-iterator/go"
)

var md5Passwd []byte

func AESBase64Encrypt(origin_data string, key []byte, iv []byte) (base64_result string, err error) {

	var block cipher.Block
	if block, err = aes.NewCipher(key); err != nil {
		return
	}
	encrypt := cipher.NewCBCEncrypter(block, iv)
	var source []byte = PKCS5Padding([]byte(origin_data), 16)
	var dst []byte = make([]byte, len(source))
	encrypt.CryptBlocks(dst, source)
	base64_result = base64.StdEncoding.EncodeToString(dst)
	return
}

func AESBase64Decrypt(encrypt_data string, key []byte, iv []byte) (origin_data string, err error) {
	var block cipher.Block
	defer func() {
		if Derr := recover(); Derr != nil {
			err = fmt.Errorf("%v", Derr)
		}
	}()
	if block, err = aes.NewCipher(key); err != nil {
		return
	}
	encrypt := cipher.NewCBCDecrypter(block, iv)

	var source []byte
	if source, err = base64.StdEncoding.DecodeString(encrypt_data); err != nil {
		return
	}
	var dst []byte = make([]byte, len(source))
	encrypt.CryptBlocks(dst, source)
	origin_data_bytes, err := PKCS5UnPadding(dst, 16)
	if err != nil {
		return
	}
	origin_data = string(origin_data_bytes)
	return
}

func PKCS5Padding(ciphertext []byte, blockSize int) []byte {
	padding := blockSize - len(ciphertext)%blockSize
	padtext := bytes.Repeat([]byte{byte(padding)}, padding)
	return append(ciphertext, padtext...)
}

func PKCS5UnPadding(data []byte, blocklen int) ([]byte, error) {
	if blocklen <= 0 {
		return nil, fmt.Errorf("invalid blocklen %d", blocklen)
	}
	if len(data)%blocklen != 0 || len(data) == 0 {
		return nil, fmt.Errorf("invalid data len %d", len(data))
	}
	padlen := int(data[len(data)-1])
	if padlen > blocklen || padlen == 0 {
		return nil, fmt.Errorf("invalid padding")
	}
	pad := data[len(data)-padlen:]
	for i := 0; i < padlen; i++ {
		if pad[i] != byte(padlen) {
			return nil, fmt.Errorf("invalid padding")
		}
	}
	return data[:len(data)-padlen], nil
}

type Param struct {
	Mode string `json:"mode"`
	Raw  string `json:"raw"`
}
type encryptPkt struct {
	Type   string `json:"type"`
	Params Param  `json:"params"`
}

func encrypt_send(str string) []byte {
	en, err := AESBase64Encrypt(str, md5Passwd[0:16], md5Passwd[16:32])
	if err != nil {
		fmt.Println(err)
	}
	pkt := encryptPkt{Params: Param{Mode: "aes_cbc_pck7padding", Raw: en}, Type: "encrypted"}
	jpkt, _ := jsoniter.Marshal(pkt)
	//fmt.Println(string(jpkt))
	return jpkt

}

type runcmdParams struct {
	Cmd string `json:"cmd"`
	Id  string `json:"id"`
}
type basePkt struct {
	Type   string      `json:"type"`
	Action string      `json:"action"`
	Params interface{} `json:"params"`
}
type Vec3 struct {
	X float32 `json:"x"`
	Y float32 `json:"y"`
	Z float32 `json:"z"`
}
type PlayerJoinPkt struct {
	Sender string `json:"sender"`
	Xuid   string `json:"xuid"`
	IP     string `json:"ip"`
}
type PlayerLeftPkt struct {
	Sender string `json:"sender"`
	Xuid   string `json:"xuid"`
}
type PlayerChatPkt struct {
	Sender string `json:"sender"`
	Text   string `json:"text"`
}
type RuncmdCallbackPkt struct {
	Result string `json:"result"`
	Id     string `json:"id"`
}
type MobDiePkt struct {
	MobType   string `json:"mobtype"`
	MobName   string `json:"mobname"`
	SrcType   string `json:"srctype"`
	SrcName   string `json:"srcname"`
	CauseCode int    `json:"dmcase"`
	CauseName string `json:"dmname"`
	Pos       Vec3   `json:"pos"`
}

func actionSwitch(act map[string]interface{}) {
	if pkttype, ok := act["type"].(string); ok && pkttype == "pack" {
		if action, ok := act["cause"].(string); ok {
			switch action {
			case "join":
				resByte, _ := jsoniter.Marshal(act["params"])
				var data PlayerJoinPkt
				err := jsoniter.Unmarshal(resByte, &data)
				if err != nil {
					return
				}
				fmt.Printf("%+v\n", data)
			case "left":
				resByte, _ := jsoniter.Marshal(act["params"])
				var data PlayerLeftPkt
				err := jsoniter.Unmarshal(resByte, &data)
				if err != nil {
					return
				}
				fmt.Printf("%+v\n", data)
			case "chat":
				resByte, _ := jsoniter.Marshal(act["params"])
				var data PlayerChatPkt
				err := jsoniter.Unmarshal(resByte, &data)
				if err != nil {
					return
				}
				fmt.Printf("%+v\n", data)
			case "mobdie":
				resByte, _ := jsoniter.Marshal(act["params"])
				var data MobDiePkt
				err := jsoniter.Unmarshal(resByte, &data)
				if err != nil {
					return
				}
				fmt.Printf("%+v\n", data)
			case "runcmdfeedback":
				resByte, _ := jsoniter.Marshal(act["params"])
				var data RuncmdCallbackPkt
				err := jsoniter.Unmarshal(resByte, &data)
				if err != nil {
					return
				}
				fmt.Println(act["params"].(map[string]interface{})["result"])
			default:
				fmt.Println("NO Such Action", action)
			}
		}
	} else {
		fmt.Println("MutiEncrypted Pakcet!!!")
		return
	}
}
func wsHandler(Connection *websocket.Conn) {
	done := make(chan struct{})
	defer close(done)
	for {
		_, message, err := Connection.ReadMessage()
		if err != nil {
			log.Println("read:", err)
			return
		}
		log.Printf("recv: %s", message)
		message = []byte(strings.ReplaceAll(string(message), "\\r", ""))
		var in_json map[string]interface{}
		jsoniter.Unmarshal(message, &in_json)

		if value, ok := in_json["type"].(string); ok {
			if value == "encrypted" {
				if params, ok := in_json["params"].(map[string]interface{}); ok {
					//handle switch(action)

					if mode, ok := params["mode"].(string); ok {
						switch mode {
						case "aes_cbc_pck7padding":
							if raw, ok := params["raw"].(string); ok {
								str, err := AESBase64Decrypt(raw, md5Passwd[0:16], md5Passwd[16:32])
								if err != nil {
									fmt.Println("DecryptErr", err)
								}
								str = strings.ReplaceAll(str, "\\r", "")
								var action map[string]interface{}
								jsoniter.Unmarshal([]byte(str), &action)
								fmt.Println("Parsed Msg: ", str)
								actionSwitch(action)
							} else {

								fmt.Println("JsonParseError> Require ObjectType Raw Json with at least one member")
							}
						default:
							fmt.Println("DecodeError> Encrypt mode Not Support")
						}
					}

				} else {
					fmt.Println("JsonParseError> [params] Not Found or Not a object")
				}
			} else {
				if value == "pack" {
					fmt.Println("Parsed Msg: ", in_json)
					actionSwitch(in_json)
				} else {
					fmt.Println("JsonParseError Unknow PackType")
				}
			}
		} else {
			fmt.Println("JsonParseError [type] Not Found or Not a string")
		}
	}
}
func main() {
	//interrupt := make(chan os.Signal, 1)
	//signal.Notify(interrupt, os.Interrupt)
	var addr, path, passwd string
	fmt.Print("Addr   > ")
	fmt.Scanln(&addr)
	fmt.Print("Path   > ")
	fmt.Scanln(&path)
	fmt.Print("passwd > ")
	fmt.Scanln(&passwd)
	md5 := md5.Sum([]byte(passwd))
	md5Passwd = []byte(fmt.Sprintf("%X", md5))
	fmt.Println("RealPasswd", string(md5Passwd))
	fmt.Println("Key", md5Passwd[0:16])
	fmt.Println("IV ", md5Passwd[16:32])
	u := url.URL{Scheme: "ws", Host: addr, Path: path}
	log.Printf("connecting to %s", u.String())

	Connection, _, err := websocket.DefaultDialer.Dial(u.String(), nil)
	if err != nil {
		log.Fatal("dial:", err)
	}
	defer Connection.Close()

	go wsHandler(Connection)
	reader := bufio.NewReader(os.Stdin)
	for {
		var msg string
		//fmt.Print("MinecraftCMD  > ")
		msg, _ = reader.ReadString('\n')
		msg = strings.Replace(msg, "\n", "", -1)
		Id := fmt.Sprintf("%x", rand.Intn(114514)+rand.Intn(1919810))
		pkt := basePkt{Params: runcmdParams{Cmd: msg, Id: Id}, Type: "pack", Action: "runcmdrequest"}
		jpkt, _ := jsoniter.Marshal(pkt)
		en := encrypt_send(string(jpkt))
		Connection.WriteMessage(websocket.TextMessage, en)
	}
}
