def time_to_int(time):
    time = time.split(":")
    return ((int(time[0]) * 60) + int(time[1])) * 60 + int(time[2])


with open("../gtfs/calendar.txt", "r") as file:
    services = file.readlines()[1:]
    services = [service.strip().split(",")[:6] for service in services]
    service_ids = [int(service[0]) for service in services if all(int(day) == 1 for day in service[1:])]

with open("../gtfs/trips.txt", "r") as file:
    trips = file.readlines()[1:]
    trips = [trip.replace("\"", "").replace(", ", " ") for trip in trips]
    trips = [trip.strip().split(",")[:3] for trip in trips]
    trips = {int(trip[2]): int(trip[0]) for trip in trips if int(trip[1]) in service_ids}

with open("../gtfs/stop_times.txt", "r") as file:
    stop_times = file.readlines()[1:]
    stop_times = [stop_time.strip().split(",")[:4] for stop_time in stop_times]
    stop_times = [[stop_time[0], stop_time[3]] + stop_time[1:3] for stop_time in stop_times if int(stop_time[0]) in list(trips.keys())]
    stop_times = [list(map(int, stop_time[:2])) + list(map(time_to_int, stop_time[2:])) for stop_time in stop_times]
    stop_times = sorted(stop_times, key=lambda stop_time: stop_time[-1])

with open("genAlgoImprovements.txt", "r") as file:
    lines = file.readlines()
    min_scheds = [line.strip().split("|") for line in lines if "|" in line]
    # min_scheds = [line for line in lines if int(line[-1]) == 1]


route_ids = set()
stations = {}
for stop_time in stop_times:
    trip_id = stop_time[0]
    route_id = trips[trip_id]
    route_ids.add(route_id)
    station_id = stop_time[1]
    if station_id not in stations.keys():
        stations[station_id] = []
    stations[station_id].append(stop_time)

stop_times = sorted(stations.values(), key=lambda item: len(item), reverse=True)[:500]
stop_times = [stop_time for station in stop_times for stop_time in station]
stop_times = sorted(stop_times, key=lambda stop_time: stop_time[2])

sub_route_ids = set()
for stop_time in stop_times:
    trip_id = stop_time[0]
    route_id = trips[trip_id]
    sub_route_ids.add(route_id)

route_ids = list(route_ids)
route_ids.sort()
sub_route_ids = list(sub_route_ids)
sub_route_ids.sort()

new_scheds = []
for min_sched in min_scheds:
    min_sched.reverse()
    new_sched = [min_sched.pop()]
    for route_id in route_ids:
        if route_id in sub_route_ids:
            new_sched.append(min_sched.pop())
        else:
            new_sched.append('0')
    new_sched.append('1')
    new_scheds.append("|".join(new_sched))

with open('schedules.txt', 'w') as outfile:
    outfile.write("\n".join(new_scheds))
