with open("calendar.txt", "r") as file:
    services = file.readlines()[1:]
    services = [service.split(",")[:-2] for service in services]
    services = dict(zip([service[0] for service in services], services))

with open("trips.txt", "r") as file:
    trips = file.readlines()[1:]
    trips = [trip.strip().replace("\"", "").replace(", ", " ") for trip in trips]
    trips = [trip.strip().split(",") for trip in trips]
    trips = [[trip[0]] + services[trip[1]] + trip[2:] for trip in trips]
    trips = dict(zip([trip[9] for trip in trips], trips))

with open("stops.txt", "r") as file:
    stations = file.readlines()[1:]
    stations = [station.strip().split(",\"\",\"") for station in stations]
    stations = [[station[0]] + station[1].split("\"")[:1] for station in stations]
    stations = dict(zip([station[0] for station in stations], stations))

with open("stop_times.txt", "r") as file:
    stop_times = file.readlines()[1:]
    stop_times = [stop_time.strip().split(",") for stop_time in stop_times]
    stop_times = [trips[stop_time[0]] + stop_time[1:3] + stations[stop_time[3]] + stop_time[4:] for stop_time in stop_times]

with open("full_info.txt", "w") as file:
    stop_times = [' | '.join(stop_time) for stop_time in stop_times]
    file.write('\n'.join(stop_times))
