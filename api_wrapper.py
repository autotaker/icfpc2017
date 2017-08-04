#!/usr/bin/python3
# -*- encoding: utf-8 -*-
import argparse
import socket
import os
from subprocess import Popen, PIPE, TimeoutExpired
import json
import requests
import time

g_game_state = None

def extract_state(json_str):
  global g_game_state
  data = json_str[json_str.find(":") + 1:]
  print("state_data: {}".format(data))
  state_json = json.loads(data)["state"]
  print("state_json: {}".format(state_json))
  g_game_state = json.loads(state_json)

def get_host():
  #return "0.0.0.0"
  return "punter.inf.ed.ac.uk"

def get_port():
  #return 8080
  return 9007

# Utils
def str2msg(json_str):
  return "{}:{}".format(len(json_str), json_str).encode()

def send_json(sock, data):
  json_str = json.dumps(data)
  send_data = str2msg(json_str)
  sent = sock.send(send_data)
  print("send_json:{}".format(send_data))
  return sent == len(send_data)

def send_str(sock, send_data):
  sent = sock.send(send_data.encode())
  print("send_str:{}".format(send_data))
  return sent == len(send_data)

def recieve_json(sock):
  MAX_SIZE = 2048

  res = ""
  while True:
    c = sock.recv(1).decode("utf-8")
    if c == ":":
      break
    res += c
  length = int(res)
  data = ""
  #print("data: {}".format(data))
  while(len(data) < length):
    #print("data: {}".format(data))
    recv = sock.recv(min(MAX_SIZE, length - len(data))).decode("utf-8")
    assert(len(recv) != 0)
    print("lenelen: {}".format(len(recv)))
    data += recv

  print("recieve_json:{}".format(data))
  print("punter in data:{}".format("punter" in json.loads(data)))
  return json.loads(data)

def communicate_with_ai(cmd, json_str):
  data = str2msg(json_str)
  print("communicate:{}".format(data))
  TIME_OUT = 10
  p = Popen(cmd, bufsize=2048, stdin=PIPE, stdout=PIPE, stderr=PIPE)

  try:
    raw_output, raw_err = p.communicate(input=data, timeout=TIME_OUT)
    output = raw_output.decode("utf-8")
    err = raw_err.decode("utf-8")
    print("[DEBUG] STDERR: " + err)
  except TimeoutExpired:
    p.kill()
    output = 'TIMEOUT'
  except:
    print(output)
    assert(False)

  extract_state(output)

  return output

def handshake(sock, name):
  data = {"me": name}
  assert(send_json(sock, data))
  recv = recieve_json(sock)
  assert(recv["you"] == name)

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("-c", "--command", type=str, required=True, help="command to execute AI")
  parser.add_argument("-n", "--name", type=str, help="user name", default="user_name")
  args = parser.parse_args()

  USER_NAME = args.name
  CMD = args.command

  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  sock.connect((get_host(), get_port()))

  handshake(sock, USER_NAME)

  # Setting Up
  recv_str= json.dumps(recieve_json(sock))
  ready = communicate_with_ai(CMD, recv_str)
  assert(send_str(sock, ready))

  # Game
  while True:
    recv_json = recieve_json(sock)
    if "stop" in recv_json:
      break

    recv_json["state"] = json.dumps(g_game_state)
    recv_str= json.dumps(recv_json)
    move = communicate_with_ai(CMD, recv_str)
    assert(send_str(sock, move))

  sock.close()
  print("Score: {}".format(recv_json))
  print("Finish!!")

  return

if __name__ == "__main__":
  main()
