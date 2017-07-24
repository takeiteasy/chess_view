#!/usr/local/bin/python3
import chess.pgn, socket, sys
from time import sleep

with open("res/test.pgn") as pgn:
  game = chess.pgn.read_game(pgn)

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('localhost', 8888)
sock.connect(server_address)

while not game.is_end():
  next_node = game.variations[0]
  sock.sendall(str.encode(game.board().fen()))
  game = next_node
  sleep(1)

sock.close()
