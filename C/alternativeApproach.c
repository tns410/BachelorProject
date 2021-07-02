#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_NR_STATION_STOPS 1730
#define MAX_NR_TRIP_STOPS 38
#define MAX_NR_TRIPS 509
#define MAX_NR_ROUTES 495
#define MAX_NR_STATIONS 500

struct stop {
    int route_id;
    int station_id;
    int dep_time;
    int arr_time;
};

struct trip {
    int num_stops;
    struct stop stops[MAX_NR_TRIP_STOPS];
};

struct route {
    int max_headway;
    int num_trips;
    struct trip trips[MAX_NR_TRIPS];
};

struct station {
    int num_in_stops;
    int num_out_stops;
    struct stop in_stops[MAX_NR_STATION_STOPS];
    struct stop out_stops[MAX_NR_STATION_STOPS];
    struct stop walks[MAX_NR_STATIONS];
};

struct route * read_routes() {
    struct route * routes = calloc(MAX_NR_ROUTES, sizeof(struct route));
    FILE* stream = fopen("../routes.txt", "r");
    int prev_trip_idx = -1;
    char line[64];
    while(fgets(line, 64, stream)) {
        int route_idx = atoi(strtok(line, "|"));
        struct route * route = &routes[route_idx];
        route->max_headway = atoi(strtok(NULL, "|"));
        int trip_idx = atoi(strtok(NULL, "|"));
        if (trip_idx != prev_trip_idx) {
            route->num_trips++;
            prev_trip_idx = trip_idx;
        }
        struct trip * trip = &route->trips[route->num_trips-1];
        trip->num_stops++;
        struct stop * stop = &trip->stops[trip->num_stops-1];
        stop->route_id = route_idx;
        stop->station_id = atoi(strtok(NULL, "|"));
        stop->arr_time = atoi(strtok(NULL, "|"));
        stop->dep_time = atoi(strtok(NULL, "\n"));
    }
    return routes;
}

struct station * read_stations() {
    struct station * stations = calloc(MAX_NR_STATIONS, sizeof(struct station));
    FILE* stream = fopen("../stations.txt", "r");
    char line[64];
    while(fgets(line, 64, stream)) {
        int dep_station = atoi(strtok(line, "|"));
        struct station * station = &stations[dep_station];
        int arr_station = atoi(strtok(NULL, "|"));
        struct stop * stop = &station->walks[arr_station];
        stop->route_id = -1;
        stop->station_id = arr_station;
        stop->arr_time = atoi(strtok(NULL, "|"));
    }
    return stations;
}

struct station * copy_stations(struct station stations[]) {
    struct station * new_stations = calloc(MAX_NR_STATIONS, sizeof(struct station));
    memcpy(new_stations, stations, MAX_NR_STATIONS * sizeof(struct station));
    return new_stations;
}

int * init_offsets() {
    return calloc(MAX_NR_ROUTES, sizeof(int));
}

int * copy_offsets(int offsets[]) {
    int * new_offsets = calloc(MAX_NR_ROUTES, sizeof(int));
    memcpy(new_offsets, offsets, MAX_NR_ROUTES * sizeof(int));
    return new_offsets;
}

int compare_stops (const void * stop1, const void * stop2) { // cannot do stop1 - stop2 because of uint32
    struct stop * stop1_struct = (struct stop *) stop1;
    struct stop * stop2_struct = (struct stop *) stop2;
    if (stop1_struct->dep_time == 0) {
        if (stop1_struct->arr_time < stop2_struct->arr_time) return -1;
        if (stop1_struct->arr_time > stop2_struct->arr_time) return  1;
    }
    else {
        if (stop1_struct->dep_time < stop2_struct->dep_time) return -1;
        if (stop1_struct->dep_time > stop2_struct->dep_time) return  1;
    }
    return 0;
}

struct station * build_network(struct route routes[], struct station stations[], const int offsets[]) {
    struct station * network = copy_stations(stations);
    for (int route_idx = 0; route_idx < MAX_NR_ROUTES; ++route_idx) {
        struct route * route = &routes[route_idx];

        for (int trip_idx = 0; trip_idx < route->num_trips; ++trip_idx) {
            struct trip * trip = &route->trips[trip_idx];

            for (int stop_idx = 0; stop_idx < trip->num_stops-1; ++stop_idx) {
                struct stop * from_stop = &trip->stops[stop_idx];
                struct stop * to_stop = &trip->stops[stop_idx+1];

                struct station * out_station = &network[from_stop->station_id];
                struct stop * out_stop = &out_station->out_stops[out_station->num_out_stops++];
                out_stop->route_id = from_stop->route_id;
                out_stop->dep_time = from_stop->dep_time + offsets[route_idx] * 60;
                out_stop->station_id = to_stop->station_id;

                struct station * in_station = &network[to_stop->station_id];
                struct stop * in_stop = &in_station->in_stops[in_station->num_in_stops++];
                in_stop->route_id = to_stop->route_id;
                in_stop->arr_time = to_stop->arr_time + offsets[route_idx] * 60;
                in_stop->station_id = from_stop->station_id;
            }
        }
    }

    for (int idx = 0; idx < MAX_NR_STATIONS; ++idx) {
        qsort(network[idx].out_stops, network[idx].num_out_stops, sizeof(struct stop), compare_stops);
        qsort(network[idx].in_stops, network[idx].num_in_stops, sizeof(struct stop), compare_stops);
    }

    return network;
}

bool new_station_id(struct stop * out_stop, struct stop * in_stop, const int station_ids[], int num_station_ids) {
    if (out_stop->dep_time < in_stop->arr_time) return false;
    if (out_stop->station_id == in_stop->station_id) return false;
    for (int station_ids_idx = 0; station_ids_idx < num_station_ids; ++station_ids_idx) {
        if (in_stop->station_id == station_ids[station_ids_idx]) return false;
    }
    return true;
}

float calculate_network_efficiency(struct station network[]) {
    long total_transfer_sum = 0;
    long num_total_transfer = 0;
    int station_ids[MAX_NR_STATION_STOPS];

    for (int station_idx = 0; station_idx < MAX_NR_STATIONS; ++station_idx) {

        struct station * station = &network[station_idx];
        for (int out_stop_idx = station->num_out_stops - 1; out_stop_idx >= 0; out_stop_idx--) {
            int num_station_ids = 0;
            struct stop * out_stop = &station->out_stops[out_stop_idx];
            if (out_stop->dep_time > 8 * 60 * 60 && out_stop->dep_time < 20 * 60 * 60) {

                for (int in_stop_idx = station->num_in_stops - 1; in_stop_idx >= 0; in_stop_idx--) {
                    struct stop * in_stop = &station->in_stops[in_stop_idx];

                    if (new_station_id(out_stop, in_stop, station_ids, num_station_ids)) {
                        station_ids[num_station_ids++] = in_stop->station_id;
                        total_transfer_sum += out_stop->dep_time - in_stop->arr_time;
                        num_total_transfer++;
                    }
                }
            }
        }
    }
    return (float)total_transfer_sum / (float)num_total_transfer;
}

int main()
{
    struct route * routes = read_routes();
    struct station * stations = read_stations();
    int * offsets = init_offsets();

    for (int route_id = 0; route_id < 5; ++route_id) {
        for (int offset = -20; offset < 21; ++offset) {
            offsets[0] = offset;
            struct station * network = build_network(routes, stations, offsets);
            float efficiency = calculate_network_efficiency(network);
            printf("%d, 0, %d, %f\n", route_id, offset, efficiency);
        }
    }
}
