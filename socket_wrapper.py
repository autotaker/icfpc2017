#!/usr/bin/python3
# -*- encoding: utf-8 -*-
import argparse
import socket
import os
from subprocess import Popen, PIPE, TimeoutExpired
import json

g_game_state = None
g_host_name = "punter.inf.ed.ac.uk"

def cutoff_handshake(s):
  return s[s.find("\n") + 1:]

def extract_state(s):
  global g_game_state
  data = s[s.find(":") + 1:]
  state_json = json.loads(data)["state"]
  g_game_state = state_json
  return s

# Utils
def str2msg(json_str):
  return "{}:{}".format(len(json_str), json_str).encode()

def my_send(sock, send_data):
  sent = 0
  while(sent < len(send_data)):
    cnt = sock.send(send_data[sent:])
    assert(cnt > 0)
    sent += cnt

def send_json(sock, data):
  json_str = json.dumps(data)
  send_data = str2msg(json_str)
  my_send(sock, send_data)

def send_str(sock, send_data):
  my_send(sock, send_data.encode())

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
  while(len(data) < length):
    recv = sock.recv(min(MAX_SIZE, length - len(data))).decode("utf-8")
    assert(len(recv) != 0)
    data += recv

  return json.loads(data)

def communicate_with_ai(cmd, json_str):
  data = str2msg(json.dumps({"you": "name"})) + str2msg(json_str)
  TIME_OUT = 1
  p = Popen(cmd, bufsize=2048, stdin=PIPE, stdout=PIPE, stderr=PIPE)

  try:
    raw_output, raw_err = p.communicate(input=data, timeout=TIME_OUT)
    output = raw_output.decode("utf-8")
    err = raw_err.decode("utf-8")
    print("STDERR:\n" + err)
  except TimeoutExpired:
    p.kill()
    output = 'TIMEOUT'
  except:
    print(output)
    assert(False)

  s = cutoff_handshake(output)
  extract_state(s)

  return s

def handshake(sock, name):
  data = {"me": name}
  send_json(sock, data)
  print("handshake:send {}".format(data))
  recv = recieve_json(sock)
  assert(recv["you"] == name)

def main():
  parser = argparse.ArgumentParser(description='e.g. %(prog)s -c "./cympfh/offline_ai.py" -p 9007')
  parser.add_argument("-c", "--command", type=str, required=True, help="command to execute your AI")
  parser.add_argument("-n", "--name", type=str, help="user name", default="user_name")
  parser.add_argument("-s", "--server", type=str, help="Server URL", default="punter.inf.ed.ac.uk")
  parser.add_argument("-p", "--port", type=int, required=True, help="port number")
  args = parser.parse_args()

  USER_NAME = args.name
  CMD = args.command
  HOST_NAME = args.server
  PORT = args.port

  print("Try to connect '{}:{}'".format(HOST_NAME, PORT))

  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  sock.connect((HOST_NAME, PORT))
  print("connected!")

  handshake(sock, USER_NAME)

  # Setting Up
  setting_json = recieve_json(sock)
  my_id = setting_json["punter"]
  recv_str= json.dumps(setting_json)
  ready = communicate_with_ai(CMD, recv_str)
  send_str(sock, ready)

  # Game
  while True:
    recv_json = recieve_json(sock)
    if "stop" in recv_json:
      break

    recv_json["state"] = g_game_state
    recv_str= json.dumps(recv_json)
    move = communicate_with_ai(CMD, recv_str)
    send_str(sock, move)

  sock.close()
  print("=Result=")
  print("My punter ID = {}".format(my_id))
  print("Score: {}".format(recv_json["stop"]["scores"]))
  print("Finish!!")

if __name__ == "__main__":
  main()
