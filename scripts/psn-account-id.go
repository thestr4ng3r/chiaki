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
	"time"
	"fmt"
	"log"
	"os"
)


func main(){
	fmt.Print(
`
PSN ID Scraper for Remote Access
Press any key to open the PSN Login Page.

When you see "Redirect" paste the URL from your browser back here, then press Enter
`)

	browser.OpenURL("https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize?service_entity=urn:service-entity:psn&response_type=code&client_id=ba495a24-818c-472b-b12d-ff231c1b5745&redirect_uri=https://remoteplay.dl.playstation.net/remoteplay/redirect&scope=psn:clientapp&request_locale=en_US&ui=pr&service_logo=ps&layout_type=popup&smcid=remoteplay&prompt=always&PlatformPrivacyWs1=minimal&")
	reader := bufio.NewReader(os.Stdin)
	response, _ := reader.ReadString('\n')
	response = strings.TrimSpace(response)
	URL, err := url.Parse(response)
	if err != nil {
		log.Fatal(err)
	}

	query := URL.Query()

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
	uid, _ := strconv.ParseInt(info.UserID, 10, 32)
	
	buf := new(bytes.Buffer)
	err = binary.Write(buf, binary.LittleEndian, uid) //TODO: WRONG!
	if err != nil {
		log.Fatal(err)
	}

	uidBytes := buf.Bytes()
	fmt.Printf("Your AccountID is: %v (Copied to Clipboard)", base64.StdEncoding.EncodeToString(uidBytes))
	clipboard.WriteAll(base64.StdEncoding.EncodeToString(uidBytes))

}

type AccessKeys struct {
	AccessToken		string	`json:"access_token"`
	TokenType		string	`json:"token_type"`
	RefreshToken	string	`json:"refresh_token"`
	ExpiresIn		int		`json:"expires_in"`
	Scope			string	`json:"scope"`
}

type ClientInfo struct {
	Scopes			string		`json:"scopes"`
	Expiration		time.Time	`json:"expiration"`
	ClientID		string		`json:"client_id"`
	DcimID			string		`json:"dcim_id"`
	GrantType		string		`json:"grant_type"`
	UserID			string		`json:"user_id"`
	UserUUID		string		`json:"user_uuid"`
	OnlineID		string		`json:"online_id"`
	CountryCode		string		`json:"country_code"`
	LanguageCode	string		`json:"language_code"`
	CommunityDomain	string		`json:"community_domain"`
	IsSubAccount	bool		`json:"is_sub_account"`
}
