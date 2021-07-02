import math
import requests
import json


def dist(stop1, stop2):
    return math.sqrt((stop2[1] - stop1[1]) ** 2 + (stop2[2] - stop1[2]) ** 2)


def format_num(num):
    return format(num, ".5f")


def format_url(stop1, stop2):
    url = 'http://localhost:5000/route/v1/foot/{},{};{},{}?annotations=true'
    return url.format(format_num(stop1[2]), format_num(stop1[1]), format_num(stop2[2]), format_num(stop2[1]))


def get_time(stop1, stop2):
    url = format_url(stop1, stop2)
    page = requests.get(url)
    obj = json.loads(page.content.decode('utf-8'))
    seconds = obj['routes'][0]['duration']
    return math.ceil(seconds)


with open("stops.txt", "r") as file:
    stops = file.readlines()[1:]
    stops = [stop.split("\"") for stop in stops]
    stops = [[int(stop[0].replace(",", "")), float(stop[5]), float(stop[7])] for stop in stops]
    stops = sorted(stops, key=lambda stop: int(stop[0]))

with open("walk_times.txt", "r") as file:
    walk_times = file.readlines()[1:]
    walk_times = [walk_time.strip().split(",") for walk_time in walk_times]
    walk_times = [[int(item) for item in walk_time] for walk_time in walk_times]
    walks = {}
    for walk_time in walk_times:
        if walk_time[0] not in walks:
            walks[walk_time[0]] = {}
        walks[walk_time[0]][walk_time[1]] = walk_time[2]
        x = 0

times = []
for idx, dep_stop in enumerate(stops):
    for arr_stop in stops:
        if dep_stop[0] in walks.keys() and arr_stop[0] in walks[dep_stop[0]].keys():
            times.append("{},{},{}".format(dep_stop[0], arr_stop[0], walks[dep_stop[0]][arr_stop[0]]))
        else:
            times.append("{},{},{}".format(dep_stop[0], arr_stop[0], str(get_time(dep_stop, arr_stop))))

with open("walk_times_new.txt", "w") as file:
    file.write("\"dep_stop_id\",\"arr_stop_id\",\"walk_time\"\n")
    file.write("\n".join(times))
