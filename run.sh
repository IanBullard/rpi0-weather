#!/bin/bash
cd "$(dirname "$0")"

while true
do
  python app.py
  sleep 10
done