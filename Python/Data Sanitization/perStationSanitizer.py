def time_to_int(time):
    time = time.split(":")
    return ((int(time[0]) * 60) + int(time[1])) * 60 + int(time[2])


def calc_headway(start_times):
    maximum = 0
    for idx in range(len(start_times) - 1):
        diff = start_times[idx+1] - start_times[idx]
        maximum = max(maximum, abs(diff))
    return maximum


with open("calendar.txt", "r") as file:
    services = file.readlines()[1:]
    services = [service.strip().split(",")[:6] for service in services]
    service_ids = [int(service[0]) for service in services if all(int(day) == 1 for day in service[1:])]

with open("trips.txt", "r") as file:
    trips = file.readlines()[1:]
    trips = [trip.replace("\"", "").replace(", ", " ") for trip in trips]
    trips = [trip.strip().split(",")[:3] for trip in trips]
    trips = {int(trip[2]): int(trip[0]) for trip in trips if int(trip[1]) in service_ids}

with open("stop_times.txt", "r") as file:
    stop_times = file.readlines()[1:]
    stop_times = [stop_time.strip().split(",")[:4] for stop_time in stop_times]
    stop_times = [[stop_time[0], stop_time[3]] + stop_time[1:3] for stop_time in stop_times if int(stop_time[0]) in list(trips.keys())]
    stop_times = [list(map(int, stop_time[:2])) + list(map(time_to_int, stop_time[2:])) for stop_time in stop_times]
    stop_times = sorted(stop_times, key=lambda stop_time: stop_time[-1])

with open("walk_times.txt", "r") as file:
    walk_times = file.readlines()[1:]
    walk_times = [walk_time.strip().split(",") for walk_time in walk_times]
    walk_times = [[int(item) for item in walk_time] for walk_time in walk_times if int(walk_time[-1]) < 10*60]


routes = {}
stations = {}
for stop_time in stop_times:
    trip_id = stop_time[0]
    route_id = trips[trip_id]
    station_id = stop_time[1]
    if route_id not in routes.keys():
        routes[route_id] = {}
    if trip_id not in routes[route_id].keys():
        routes[route_id][trip_id] = []
    routes[route_id][trip_id].append(stop_time)
    if station_id not in stations.keys():
        stations[station_id] = []
    stations[station_id].append(stop_time)

headways = {}
for route_id, tmp_trips in routes.items():
    dirs = {}
    for trip in tmp_trips.values():
        stops = tuple([stop[1] for stop in trip])
        if stops not in dirs.keys():
            dirs[stops] = []
        dirs[stops].append(trip)
    trip = sorted(dirs.values(), key=lambda item: len(item), reverse=True)[0]
    headways[route_id] = calc_headway([trip[0][-1] for trip in trip])
    if headways[route_id] == 0:
        headways[route_id] = 12 * 60 * 60

stop_times = sorted(stations.values(), key=lambda item: len(item), reverse=True)[:500]
stop_times = [stop_time for station in stop_times for stop_time in station]
stop_times = sorted(stop_times, key=lambda stop_time: stop_time[2])

routes = {}
stations = {}
for stop_time in stop_times:
    trip_id = stop_time[0]
    route_id = trips[trip_id]
    station_id = stop_time[1]
    if route_id not in routes.keys():
        routes[route_id] = {}
    if trip_id not in routes[route_id].keys():
        routes[route_id][trip_id] = []
    routes[route_id][trip_id].append(stop_time[1:])
    if station_id not in stations.keys():
        stations[station_id] = []
    stations[station_id].append(stop_time)

route_ids = list(routes.keys())
route_ids.sort()
station_ids = list(stations.keys())
station_ids.sort()

MAX_NR_STATION_STOPS = max([len(item) for item in stations.values()])
MAX_NR_TRIP_STOPS = 0
MAX_NR_TRIPS = 0
trip_num = 0
stop_times_opt = []
route_nums = {route_id: 0 for route_id in route_ids}
for idx, route_id in enumerate(route_ids):
    route = routes[route_id]
    MAX_NR_TRIPS = max(MAX_NR_TRIPS, len(route))
    for trip_id, trip in route.items():
        MAX_NR_TRIP_STOPS = max(MAX_NR_TRIP_STOPS, len(trip))
        for stop in trip:
            stop_times_opt.append([idx, headways[route_id], trip_num] + stop)
        trip_num += 1
        if len(trip) > 1:
            route_nums[route_id] += 1
route_nums = [route_id for route_id, num in route_nums.items() if num > 0]

stations = {station_id: idx for idx, station_id in enumerate(station_ids)}

walk_times_opt = [walk_time for walk_time in walk_times if walk_time[0] in station_ids and walk_time[1] in station_ids]
walk_times = {}
for walk_time in walk_times_opt:
    if walk_time[0] not in walk_times.keys():
        walk_times[walk_time[0]] = []
    walk_times[walk_time[0]].append(walk_time)
walk_times = [len(station) for station in walk_times.values()]
walk_times_opt = [[stations[walk_time[0]], stations[walk_time[1]], walk_time[2], walk_time[0]] for walk_time in walk_times_opt]
walk_times_opt = ["|".join(map(str, walk_time)) for walk_time in walk_times_opt]

stop_times_opt = [time[:3] + [stations[time[3]]] + time[4:] for time in stop_times_opt]
stop_times_opt = ["|".join(map(str, stop)) for stop in stop_times_opt]

print("#define MAX_NR_STATION_STOPS " + str(MAX_NR_STATION_STOPS))
print("#define MAX_NR_STATION_WALKS " + str(max(walk_times)))
print("#define MAX_NR_TRIP_STOPS " + str(MAX_NR_TRIP_STOPS))
print("#define MAX_NR_TRIPS " + str(MAX_NR_TRIPS))
print("#define MAX_NR_ROUTES " + str(len(routes)))
print("#define MAX_NR_STATIONS " + str(len(station_ids)))
print("        MAX_NR_ROUTES_OPT " + str(len(route_nums)))
print("        MAX_NR_STOPS " + str(len(stop_times_opt)))
print("        MAX_NR_WALKS " + str(len(walk_times_opt)))

with open('routes.txt', 'w') as outfile:
    outfile.write("\n".join(stop_times_opt))

with open('stations.txt', 'w') as outfile:
    outfile.write("\n".join(walk_times_opt))
