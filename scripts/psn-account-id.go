package main

import (
	"github.com/atotto/clipboard"
	"github.com/pkg/browser"
	"encoding/base64"
	"encoding/binary"
	"encoding/json"
	"net/http"
	"net/url"
	"strings"
	"strconv"
	"bytes"
	"bufio"
	"flag"
	"fmt"
	"log"
	"os"
)

func main(){
	headless := flag.Bool("headless", false, "Operates in Headless mode")
	flag.Parse()
	fmt.Println(
`== PSN ID Scraper for Remote Play ==
In order to get your Account code for Remote Play, You'll need to Login via a special Remote Play login webpage.
After logging in, you will see a webpage that displays "redirect" in the top-left.
When you see this page, Copy the entire URL from your browser, paste it below and then press *Enter*
`)
	reader := bufio.NewReader(os.Stdin)
	if *headless {
		fmt.Println(
`[Headless] You'll need to open this page in a web browser that supports Javascript/ReCaptcha
[Headless] https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize?service_entity=urn:service-entity:psn&response_type=code&client_id=ba495a24-818c-472b-b12d-ff231c1b5745&redirect_uri=https://remoteplay.dl.playstation.net/remoteplay/redirect&scope=psn:clientapp&request_locale=en_US&ui=pr&service_logo=ps&layout_type=popup&smcid=remoteplay&prompt=always&PlatformPrivacyWs1=minimal&`)
	} else {
		fmt.Println("Press Enter to open the PSN Remote Play login webpage in your browser")
		reader.ReadString('\n')
		browser.OpenURL("https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize?service_entity=urn:service-entity:psn&response_type=code&client_id=ba495a24-818c-472b-b12d-ff231c1b5745&redirect_uri=https://remoteplay.dl.playstation.net/remoteplay/redirect&scope=psn:clientapp&request_locale=en_US&ui=pr&service_logo=ps&layout_type=popup&smcid=remoteplay&prompt=always&PlatformPrivacyWs1=minimal&")
	}

	fmt.Print("Awaiting Input >")
	response, _ := reader.ReadString('\n')
	response = strings.TrimSpace(response)
	URL, err := url.Parse(response)
	if err != nil {
		log.Fatal(err)
	}

	query := URL.Query()
	if query.Get("code") == ""{
		log.Fatal("Invalid URL has been submitted")
	}

	client := &http.Client{}
	data :=url.Values{}
	data.Set("grant_type","authorization_code")
	data.Set("code", query.Get("code"))
	data.Set("redirect_uri","https://remoteplay.dl.playstation.net/remoteplay/redirect")
	req, err := http.NewRequest("POST", "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token", strings.NewReader(data.Encode()))
	req.SetBasicAuth("ba495a24-818c-472b-b12d-ff231c1b5745", "mvaiZkRsAsI1IBkY")
	req.Header.Add("Content-Type", "application/x-www-form-urlencoded")
	req.Header.Add("Content-Length", strconv.Itoa(len(data.Encode())))
	res, err := client.Do(req)

	defer res.Body.Close()
	access := AccessKeys{}
	err = json.NewDecoder(res.Body).Decode(&access)
	if err != nil {
		log.Fatal(err)
	}

	req, err = http.NewRequest("GET", fmt.Sprintf("https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token/%v", access.AccessToken), nil)
	req.SetBasicAuth("ba495a24-818c-472b-b12d-ff231c1b5745", "mvaiZkRsAsI1IBkY")
	res, err = client.Do(req)

	info := ClientInfo{}
	err = json.NewDecoder(res.Body).Decode(&info)
	if err != nil {
		log.Fatal(err)
	}
	uid, _ := strconv.ParseInt(info.UserID, 10, 0)
	
	buf := new(bytes.Buffer)
	err = binary.Write(buf, binary.LittleEndian, uid)
	if err != nil {
		log.Fatal(err)
	}

	uidBytes := buf.Bytes()
	fmt.Printf("Your AccountID is: %v (Copied to Clipboard)\n", base64.StdEncoding.EncodeToString(uidBytes))
	clipboard.WriteAll(base64.StdEncoding.EncodeToString(uidBytes))

	fmt.Println("Press Enter to quit")
	reader.ReadString('\n')
}

type AccessKeys struct {
	AccessToken		string	`json:"access_token"`
}

type ClientInfo struct {
	UserID			string		`json:"user_id"`
}
