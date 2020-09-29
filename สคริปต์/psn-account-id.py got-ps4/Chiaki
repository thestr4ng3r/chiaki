#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
if sys.version_info < (3, 0, 0):
	print("DO NOT use Python 2.\nEVER.\nhttps://pythonclock.org")
	exit(1)

import platform
oldexit = exit
def exit(code):
	if platform.system() == "Windows":
		import atexit
		input("Press Enter to exit.")
	oldexit(code)

if sys.stdout.encoding.lower() == "ascii":
	import codecs
	sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer)
	sys.stderr = codecs.getwriter('utf-8')(sys.stderr.buffer)

try:
	import requests
except ImportError as e:
	print(e)
	if platform.system() == "Windows":
		from distutils.util import strtobool
		a = input("The requests module is not available. Should we try to install it automatically using pip? [y/n] ")
		while True:
			try:
				a = strtobool(a)
				break
			except ValueError:
				a = input("Please answer with y or n: ")
		if a == 1:
			import subprocess
			subprocess.call([sys.executable, "-m", "pip", "install", "requests"])
		else:
			exit(1)
	else:
		print("\"requests\" is not available. Install it with pip or your distribution's package manager.")
		exit(1)

import requests

from urllib.parse import urlparse, parse_qs, quote, urljoin
import pprint
import base64

# Remote Play Windows Client
CLIENT_ID = "ba495a24-818c-472b-b12d-ff231c1b5745"
CLIENT_SECRET = "mvaiZkRsAsI1IBkY"

LOGIN_URL = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize?service_entity=urn:service-entity:psn&response_type=code&client_id={}&redirect_uri=https://remoteplay.dl.playstation.net/remoteplay/redirect&scope=psn:clientapp&request_locale=en_US&ui=pr&service_logo=ps&layout_type=popup&smcid=remoteplay&prompt=always&PlatformPrivacyWs1=minimal&".format(CLIENT_ID)
TOKEN_URL = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token"

print()
print("########################################################")
print("           Script to determine PSN AccountID")
print("                  thanks to grill2010")
print("     (This script will perform network operations.)")
print("########################################################")
print()
print("➡️  Open the following URL in your Browser and log in:")
print()
print(LOGIN_URL)
print()
print("➡️  After logging in, when the page shows \"redirect\", copy the URL from the address bar and paste it here:")
code_url_s = input("> ")
code_url = urlparse(code_url_s)
query = parse_qs(code_url.query)
if "code" not in query or len(query["code"]) == 0 or len(query["code"][0]) == 0:
	print("☠️  URL did not contain code parameter")
	exit(1)
code = query["code"][0]

print("🌏 Requesting OAuth Token") 

api_auth = requests.auth.HTTPBasicAuth(CLIENT_ID, CLIENT_SECRET)
body = "grant_type=authorization_code&code={}&redirect_uri=https://remoteplay.dl.playstation.net/remoteplay/redirect&".format(code)

token_request = requests.post(TOKEN_URL,
	auth = api_auth,
	headers = { "Content-Type": "application/x-www-form-urlencoded" },
	data = body.encode("ascii"))

print("⚠️  WARNING: From this point on, output might contain sensitive information in some cases!")

if token_request.status_code != 200:
	print("☠️  Request failed with code {}:\n{}".format(token_request.status_code, token_request.text))
	exit(1)

token_json = token_request.json()
if "access_token" not in token_json:
	print("☠️  \"access_token\" is missing in response JSON:\n{}".format(token_request.text))
	exit(1)
token = token_json["access_token"]

print("🌏 Requesting Account Info")

account_request = requests.get(TOKEN_URL + "/" + quote(token), auth = api_auth)

if account_request.status_code != 200:
	print("☠️  Request failed with code {}:\n{}".format(account_request.status_code, account_request.text))
	exit(1)

account_info = account_request.json()
print("🥦 Received Account Info:")
pprint.pprint(account_info)

if "user_id" not in account_info:
	print("☠️  \"user_id\" is missing in response or not a string")
	exit(1)

user_id = int(account_info["user_id"])
user_id_base64 = base64.b64encode(user_id.to_bytes(8, "little")).decode()

print()
print("🍙 This is your AccountID:")
print(user_id_base64)
exit(0)

