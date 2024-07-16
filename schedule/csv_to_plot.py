#!/usr/bin/python3

import argparse
import csv
import re

import matplotlib.pyplot as plt

def add_bar(time, core, task):
  # Check if task is still running
  if task == cores[core]["task"]:
    return

  # Add bar if task was running
  if cores[core]["task"] != "":
    y = core
    height = 1.0
    width = time - cores[core]["time"]
    left = cores[core]["time"]
    color = colors[int(REGEX.match(cores[core]["task"]).group(1)) % len(colors)]
    label = cores[core]["task"]

    ax.barh(y, width, height, left=left, color=color, edgecolor="black")
    ax.text(left + width/2, y, label, ha="center", va="center", backgroundcolor="black", color="w")

  # Start tracking next task
  cores[core]["time"] = time
  cores[core]["task"] = task

parser = argparse.ArgumentParser()
parser.add_argument("csvfile", help="File to read schedule CSV from")
args = parser.parse_args()

fig, ax = plt.subplots()

# For extracting the number from the task name
# E.g., extracts "1" from "I1_LD_Y", "4" from "P4_L1_FLUSH_Y", etc.
REGEX = re.compile("^[IP]([0-9]+).*$")
colors = [ "r", "g", "b", "c", "m", "y" ]

CORE0 = 0
CORE1 = 1
HWACC = 2

cores = [
  { "time": 0.0, "task": "" },
  { "time": 0.0, "task": "" },
  { "time": 0.0, "task": "" },
]

with open(args.csvfile) as csvfile:
  reader = csv.reader(csvfile)
  next(reader) # Skip header
  time = 0.0
  for row in reader:
    time, core0_task, core1_task, hwacc_task = tuple(row)
    time = float(time)
    add_bar(time, CORE0, core0_task)
    add_bar(time, CORE1, core1_task)
    add_bar(time, HWACC, hwacc_task)

  # Terminate remaining tasks
  add_bar(time, CORE0, "")
  add_bar(time, CORE1, "")
  add_bar(time, HWACC, "")

ax.set_yticks([CORE0, CORE1, HWACC])
ax.set_yticklabels(["Core0", "Core1", "HW Acc"])
ax.invert_yaxis()
ax.set_xlabel("Time (ms)")
ax.set_title("Schedule")

plt.show()
