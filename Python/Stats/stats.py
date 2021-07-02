import matplotlib.pyplot as plt
import numpy as np


def plot_stop_times():
    with open("gtfs/routes.txt", "r") as file:
        stop_times = file.readlines()[1:]

    hours = range(26)
    dep_times = [0 for hour in hours]
    crt_trip_id = -1
    for stop_time in stop_times:
        stop_time = stop_time.split("|")
        trip_id = int(stop_time[2])
        dep_time = int(stop_time[5])
        if trip_id != crt_trip_id:
            dep_times[dep_time // 3600] += 1
            crt_trip_id = trip_id

    plt.figure(figsize=(10, 5))
    plt.bar(hours[4:25], dep_times[4:25], color='maroon', width=0.8)
    plt.xlabel("Hours")
    plt.xticks(hours[4::5], ["{}:00".format(str(hour)) for hour in hours[4::5]])
    plt.ylabel("Number of trips")
    plt.show()


def plot_western_eu():
    years = range(1990, 2020)
    cars = [204818, 215053, 226552, 236799, 250006, 263481, 265601, 271765, 280708, 292192, 318914, 331294, 340944,
            352207, 360749, 368659, 378304, 387871, 397453, 407791, 411443, 419154, 430097, 442187, 431245, 443250,
            453837, 466472, 482001, 496326]
    cars = [car//100 for car in cars]
    passengers = [12692, 13550, 10400, 10700, 11300, 11198, 11127, 11536, 11735, 12103, 12985, 13601, 13666, 13479,
                  13685, 14054, 14793, 16442, 17676, 17039, 17995, 18200, 19834, 20714, 21503, 22496, 22459, 22930,
                  23331, 25016]
    passengers = [passenger//10 for passenger in passengers]
    kms = [2775, 2775, 2792, 2797, 2818, 2820, 2845, 2855, 2863, 2863, 2863, 2863, 2854, 2875, 2876, 2894, 2894, 2894,
           2894, 2899, 2899, 2899, 2899, 2899, 2899, 2908, 2908, 2912, 2914, 2914]
    fig = plt.figure(figsize=(10, 5))

    plt.plot(np.asarray(years), np.asarray(cars), label='Road motor vehicles registered (x$10^{-2}$)')
    plt.plot(np.asarray(years), np.asarray(kms), label='Road network length in km')
    plt.plot(np.asarray(years), np.asarray(passengers), label='Road and rail passenger traffic (x$10^{-4}$)')
    plt.xlabel("Year")
    # plt.yscale('log')
    plt.legend()
    plt.show()


def print_scale_down_time():
    with open("stats/fullDay.txt", "r") as file:
        nums = file.readlines()
        nums = [item.strip().split(", ") for item in nums]
        nums = [[int(item) for item in elem[:-1]] + [float(elem[-1])] for elem in nums]
        fullDay = {}
        for num in nums:
            if num[0] not in fullDay.keys():
                fullDay[num[0]] = []
            fullDay[num[0]].append(num)

    with open("stats/tripleHour.txt", "r") as file:
        nums = file.readlines()
        nums = [item.strip().split(", ") for item in nums]
        nums = [[int(item) for item in elem[:-1]] + [float(elem[-1])] for elem in nums]
        singleHour = {}
        for num in nums:
            if num[0] not in singleHour.keys():
                singleHour[num[0]] = {}
            if num[1] not in singleHour[num[0]].keys():
                singleHour[num[0]][num[1]] = []
            singleHour[num[0]][num[1]].append(num)

    dic = {num: 0 for num in range(9, 21)}
    for idx in range(5):
        nums = [num[-1] for num in fullDay[idx]]
        min_idx = nums.index(min(nums))
        for jdx in range(13, 20):
            nums = [num[-1] for num in singleHour[idx][jdx]]
            if nums.index(min(nums)) == min_idx:
                print("{} {}".format(idx, jdx))
                dic[jdx] += 1
    print(str(dic.values()))


def plot_scale_down_time():
    X = range(9, 21)
    single = [0, 0, 3, 2, 3, 2, 2, 3, 3, 1, 1, 0]
    double = [0, 0, 0, 4, 3, 2, 2, 4, 4, 3, 1, 0]
    triple = [0, 0, 0, 0, 3, 2, 3, 4, 4, 4, 2, 0]

    X_axis = np.arange(len(X))
    plt.figure(figsize=(10, 5))

    plt.bar(X_axis - 0.25, single, 0.25, label='1 Hour Period')
    plt.bar(X_axis + 0.0, double, 0.25, label='2 Hour Period')
    plt.bar(X_axis + 0.25, triple, 0.25, label='3 Hour Period')

    plt.xticks(X_axis, ["{}:00".format(str(time)) for time in X])
    plt.yticks([0, 1, 2, 3, 4, 5])
    plt.xlabel("Time Frame End")
    plt.ylabel("Number of correct minimums indices")
    plt.legend()
    plt.show()


def plot_network_efficiency():
    with open("fullDay.txt", "r") as file:
        nums = file.readlines()
        nums = [item.strip().split(", ") for item in nums]
        nums = [[int(item) for item in elem[:-1]] + [float(elem[-1])] for elem in nums]
        fullDay = {}
        for num in nums:
            if num[0] not in fullDay.keys():
                fullDay[num[0]] = []
            fullDay[num[0]].append(num)

    with open("singleHour.txt", "r") as file:
        nums = file.readlines()
        nums = [item.strip().split(", ") for item in nums]
        nums = [[int(item) for item in elem[:-1]] + [float(elem[-1])] for elem in nums]
        singleHour = {}
        for num in nums:
            if num[0] not in singleHour.keys():
                singleHour[num[0]] = {}
            if num[1] not in singleHour[num[0]].keys():
                singleHour[num[0]][num[1]] = []
            singleHour[num[0]][num[1]].append(num)

    minutes_offset = range(-20, 21)
    plt.figure(figsize=(10, 5))

    plt.plot(np.asarray(minutes_offset), np.asarray([item[-1] for item in fullDay[1]]), label='full day')
    # sts = [(item + (station_to_station[0] - sts[0])) for item in sts]
    plt.plot(np.asarray(minutes_offset), np.asarray([item[-1] for item in singleHour[1][16]]), label='single hour')

    plt.xlabel("Minutes Offset")
    plt.legend()
    plt.show()


def plot_alternative_approach():
    with open("stats/fullDay.txt", "r") as file:
        nums = file.readlines()
        nums = [item.strip().split(", ") for item in nums]
        nums = [[int(item) for item in elem[:-1]] + [float(elem[-1])] for elem in nums]
        fullDay = {}
        for num in nums:
            if num[0] not in fullDay.keys():
                fullDay[num[0]] = []
            fullDay[num[0]].append(num)

    with open("stats/alternativeApproach.txt", "r") as file:
        nums = file.readlines()
        nums = [item.strip().split(", ") for item in nums]
        nums = [[int(item) for item in elem[:-1]] + [float(elem[-1])] for elem in nums]
        alternativeApproach = {}
        for num in nums:
            if num[0] not in alternativeApproach.keys():
                alternativeApproach[num[0]] = []
            alternativeApproach[num[0]].append(num)

    minutes_offset = range(-20, 21)
    plt.figure(figsize=(10, 5))

    route_id = 0
    plt.plot(np.asarray(minutes_offset), np.asarray([item[-1] for item in fullDay[route_id]]), label='original algorithm')
    fe = [(item[-1] + (fullDay[route_id][0][-1] - alternativeApproach[route_id][0][-1] - 0.641964)) for item in alternativeApproach[route_id]]
    plt.plot(np.asarray(minutes_offset), np.asarray(fe), label='alternative algorithm')

    plt.xlabel("Time schedule shift (in mins)")
    plt.legend()
    plt.show()


def plot_per_station_graph():
    num_stations = range(0, 2805, 100)

    with open("stats/perStationGraph.txt", "r") as file:
        psg = file.readlines()
        psg = [item.strip().split(",") for item in psg]
        psg = [[int(item) for item in elem] for elem in psg]

    with open("stats/perRouteGraph.txt", "r") as file:
        prg = file.readlines()
        prg = [item.strip().split(",") for item in prg]
        prg = [[int(item) for item in elem] for elem in prg]

    plt.figure(figsize=(10, 5))
    plt.plot([num[0] for num in psg], np.asarray([num[1] for num in psg]), label='Number of routes if sorted by station')
    plt.plot([num[0] for num in psg], np.asarray([num[2]/1000 for num in psg]), label='Number of stops if sorted by station')
    plt.plot([num[0] for num in prg], np.asarray([num[1] for num in prg]), label='Number of routes if sorted by trip count')
    plt.plot([num[0] for num in prg], np.asarray([num[2]/1000 for num in prg]), label='Number of stops if sorted by trip count')
    plt.xlabel("Number of stations")
    plt.ylabel("Number of routes / stops(x$10^{-3}$)")
    plt.legend()
    plt.show()


def get_improvements(file_name):
    minimums = []
    procs = []
    scheds = [[]]
    times = []
    with open(file_name, "r") as file:
        lines = file.readlines()
        lines = [item.strip().split() for item in lines]
        for line in lines:
            if line[0] == "Epoch":
                minimums.append(float(line[-1]))
                scheds.append([])
            if line[0] == "proc":
                procs.append(int(line[1]) // 60000000)
            if line[0] == "new":
                scheds[-1].append(float(line[-1]))
            if line[0] == "entire":
                times.append(int(line[1]) // 60000000)
    return minimums, procs, scheds, times


def plot_improvements():
    minimums_slo, procs_slo, scheds_slo, times_slo = get_improvements('stats/hilCliImprovements.txt')
    minimums_red, procs_red, scheds_red, times_red = get_improvements('stats/hilCliRedImprovements.txt')
    minimums_gen, procs_gen, scheds_gen, times_gen = get_improvements('stats/genAlgoImprovements.txt')

    times_slo = [int(sum(times_slo[:idx])) for idx in range(len(times_slo))]
    times_red = [int(sum(times_red[:idx])) for idx in range(len(times_red))]
    times_gen = [int(sum(times_gen[:idx])) for idx in range(len(times_gen))]

    plt.figure(figsize=(10, 5))
    plt.plot(np.asarray(times_slo), np.asarray(minimums_slo), label='hill-climbing')
    plt.plot(np.asarray(times_red), np.asarray(minimums_red), label='hill-climbing (optimized)')
    plt.plot(np.asarray(times_gen), np.asarray(minimums_gen), label='genetic algorithm')
    plt.plot(np.asarray(times_gen), np.asarray([sum(scheds)/len(scheds) for scheds in scheds_gen[:-1]]), label='genetic algorithm (average)')

    plt.ylabel("Average transfer time (sec)")
    plt.xlabel("Minutes")
    plt.legend()
    plt.show()


def plot_new_networks():
    minimums_slo, procs_slo, scheds_slo, times_slo = get_improvements('stats/hilCliImprovements.txt')
    minimums_red, procs_red, scheds_red, times_red = get_improvements('stats/hilCliRedImprovements.txt')
    minimums_gen, procs_gen, scheds_gen, times_gen = get_improvements('stats/genAlgoImprovements.txt')

    tmp_scheds_gen = scheds_gen.copy()
    for idx, scheds in enumerate(scheds_gen[1:]):
        prev_min = min(tmp_scheds_gen[idx])
        scheds_gen[idx+1] = [sched for sched in scheds if sched < prev_min]

    times_slo = [int(sum(times_slo[:idx])) for idx in range(len(times_slo))]
    times_red = [int(sum(times_red[:idx])) for idx in range(len(times_red))]
    times_gen = [int(sum(times_gen[:idx])) for idx in range(len(times_gen))]

    plt.figure(figsize=(10, 5))
    plt.plot(np.asarray(times_slo), np.asarray([len(mins) for mins in scheds_slo[1:]]), label='hill-climbing')
    plt.plot(np.asarray(times_red), np.asarray([len(mins) for mins in scheds_red[1:]]), label='hill-climbing (optimized)')
    plt.plot(np.asarray(times_gen), np.asarray([len(mins) for mins in scheds_gen[1:]]), label='genetic algorithm')

    plt.ylabel("Number of improved PTNs found")
    plt.xlabel("Minutes")
    plt.legend()
    plt.show()


def read_complete_improvements(file_name):
    with open(file_name, "r") as file:
        lines = file.readlines()
        lines = [line.strip().split(" ") for line in lines if line.startswith("Proc")]

    minimums = [[float(line[-1]), int(line[1])] for line in lines if 'calculated' in line]
    minimums = sorted(minimums, key=lambda minimum: minimum[1])
    minimums = [minimum[0] for minimum in minimums]
    minimums.reverse()
    return minimums


def plot_complete_improvements():
    minimums_slo, procs_slo, scheds_slo, times_slo = get_improvements('stats/hilCliImprovements.txt')
    minimums_red, procs_red, scheds_red, times_red = get_improvements('stats/hilCliRedImprovements.txt')
    minimums_gen, procs_gen, scheds_gen, times_gen = get_improvements('stats/genAlgoImprovements.txt')

    times_slo = [int(sum(times_slo[:idx])) for idx in range(len(times_slo))]
    times_red = [int(sum(times_red[:idx])) for idx in range(len(times_red))]
    times_gen = [int(sum(times_gen[:idx])) for idx in range(len(times_gen))]

    minimums_slo = read_complete_improvements("stats/hilCliComplete.txt")
    minimums_red = read_complete_improvements("stats/hilCliRedComplete.txt")
    minimums_gen = read_complete_improvements("stats/genAlgoComplete.txt")
    minimums_gen.reverse()

    plt.figure(figsize=(10, 5))
    plt.plot(np.asarray(times_slo), np.asarray(minimums_slo[13:]), label='hill-climbing')
    plt.plot(np.asarray(times_red), np.asarray(minimums_red[12:]), label='hill-climbing (optimized)')
    plt.plot(np.asarray(times_gen), np.asarray(minimums_gen), label='genetic algorithm')

    plt.ylabel("Average transfer time (sec)")
    plt.xlabel("Minutes")
    plt.legend()
    plt.show()


plot_per_station_graph()
