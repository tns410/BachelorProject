#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <mpi.h>

#define MAX_NR_STATION_STOPS 1730
#define MAX_NR_STATION_WALKS 33
#define MAX_NR_TRIP_STOPS 57
#define MAX_NR_TRIPS 525
#define MAX_NR_ROUTES 537
#define MAX_NR_STATIONS 2779

struct journey {
    bool done;
    int route_id;
    int dep_time;
    int num_transfers;
    int transfer_time;
};

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
    int num_trips;
    int max_headway;
    struct trip trips[MAX_NR_TRIPS];
};

struct walk {
    int walk_time;
    int station_id;
};

struct station {
    int num_walks;
    struct walk walks[MAX_NR_STATION_WALKS];
    int num_stops;
    struct stop stops[MAX_NR_STATION_STOPS];
};

struct schedule {
    float efficiency;
    int offsets[MAX_NR_ROUTES];
    bool visited;
};

struct route * read_routes() {
    struct route * routes = calloc(MAX_NR_ROUTES, sizeof(struct route));
    FILE* stream = fopen("routes.txt", "r");
    int prev_trip_idx = -1;
    char line[64];
    while(fgets(line, 64, stream)) {
        int route_idx = atoi(strtok(line, "|"));
        struct route * route = &routes[route_idx];
        route->max_headway = atoi(strtok(NULL, "|")) / 60;
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
    fclose(stream);
    return routes;
}

struct station * read_stations() {
    struct station * stations = calloc(MAX_NR_STATIONS, sizeof(struct station));
    FILE* stream = fopen("stations.txt", "r");
    char line[64];
    while(fgets(line, 64, stream)) {
        int dep_station = atoi(strtok(line, "|"));
        struct station * station = &stations[dep_station];
        struct walk * walk = &station->walks[station->num_walks++];
        walk->station_id = atoi(strtok(NULL, "|"));
        walk->walk_time = atoi(strtok(NULL, "|"));
    }
    fclose(stream);
    return stations;
}

struct schedule * read_schedules() {
    struct schedule * schedules = calloc(35, sizeof(struct schedule));
    FILE * stream = fopen("schedules.txt", "r");
    char line[4096];
    for (int i = 0; i < 35; ++i) {
        fgets(line, 4096, stream);
        struct schedule * schedule = &schedules[i];
        schedule->efficiency = atof(strtok(line, "|"));
        for (int offset_idx = 0; offset_idx < MAX_NR_ROUTES; ++offset_idx)
            schedule->offsets[offset_idx] = atoi(strtok(NULL, "|"));
        schedule->visited = atoi(strtok(NULL, "\n"));
    }
    fclose(stream);
    return schedules;
}

struct station * copy_stations(struct station stations[]) {
    struct station * new_stations = calloc(MAX_NR_STATIONS, sizeof(struct station));
    memcpy(new_stations, stations, MAX_NR_STATIONS * sizeof(struct station));
    return new_stations;
}

int compare_stops (const void * stop1, const void * stop2) { // cannot do stop1 - stop2 because of uint32
    struct stop * stop1_struct = (struct stop *) stop1;
    struct stop * stop2_struct = (struct stop *) stop2;
    if (stop1_struct->dep_time < stop2_struct->dep_time) return -1;
    if (stop1_struct->dep_time > stop2_struct->dep_time) return  1;
    return 0;
}

struct station * build_network(struct route routes[], struct station stations[], const int offsets[]) {
    struct station * network = copy_stations(stations);
    for (int route_idx = 0; route_idx < MAX_NR_ROUTES; ++route_idx) {
        struct route * route = &routes[route_idx];

        for (int trip_idx = 0; trip_idx < route->num_trips; ++trip_idx) {
            struct trip * trip = &route->trips[trip_idx];

            for (int stop_idx = 0; stop_idx < trip->num_stops - 1; ++stop_idx) {
                struct stop * from_stop = &trip->stops[stop_idx];
                struct stop * to_stop = &trip->stops[stop_idx+1];
                struct station * station = &network[to_stop->station_id];
                struct stop * station_stop = &station->stops[station->num_stops++];
                station_stop->route_id = from_stop->route_id;
                station_stop->station_id = from_stop->station_id;
                station_stop->dep_time = from_stop->dep_time + offsets[route_idx] * 60;
                station_stop->arr_time = to_stop->arr_time + offsets[route_idx] * 60;
            }
        }
    }

    for (int idx = 0; idx < MAX_NR_STATIONS; ++idx)
        qsort(network[idx].stops, network[idx].num_stops, sizeof(struct stop), compare_stops);

    return network;
}

int get_latest_idx(struct journey *journeys) {
    int latest_idx = 0;
    int latest_dep = 0;
    for (int journey_idx = 0; journey_idx < MAX_NR_STATIONS; ++journey_idx) {
        struct journey journey = journeys[journey_idx];
        if (!journey.done && journey.dep_time > latest_dep) {
            latest_idx = journey_idx;
            latest_dep = journey.dep_time;
        }
    }
    return latest_idx;
}

void update_journeys(struct journey journeys[], struct station * station, struct journey * crnt_journey) {
    for (int walk_idx = station->num_walks - 1; walk_idx >= 0; walk_idx--) {
        struct walk * walk = &station->walks[walk_idx];
        struct journey * journey = &journeys[walk->station_id];
        if (crnt_journey->route_id != MAX_NR_ROUTES) {
            int departure_time = crnt_journey->dep_time - walk->walk_time;
            if (departure_time > journey->dep_time) {
                journey->route_id = MAX_NR_ROUTES;
                journey->dep_time = departure_time;
                journey->num_transfers = crnt_journey->num_transfers;
                journey->transfer_time = crnt_journey->transfer_time;
            }
        }
    }

    for (int stop_idx = station->num_stops - 1; stop_idx >= 0 ; stop_idx--) {
        struct stop * stop = &station->stops[stop_idx];
        if (stop->arr_time <= crnt_journey->dep_time) {
            struct journey * journey = &journeys[stop->station_id];
            int dep_diff = stop->dep_time - journey->dep_time;
            if (stop->route_id != crnt_journey->route_id) dep_diff -= 60;
            if ((dep_diff > 0) || (dep_diff == 0 && crnt_journey->num_transfers < journey->num_transfers)) {
                journey->route_id = stop->route_id;
                journey->dep_time = stop->dep_time;
                journey->num_transfers = crnt_journey->num_transfers;
                journey->transfer_time = crnt_journey->transfer_time;
                journey->num_transfers += stop->route_id != crnt_journey->route_id ? 1 : 0;
                if ((stop->route_id != crnt_journey->route_id) && (crnt_journey->num_transfers > 0)) {
                    journey->transfer_time += crnt_journey->dep_time - stop->arr_time;
                }
            }
        }
    }
}

float calculate_network_efficiency(struct station network[]) {
    long total_transfer_sum = 0;
    int num_total_transfer = 0;

    for (int arr_sta_idx = 0; arr_sta_idx < MAX_NR_STATIONS; ++arr_sta_idx) {
        printf("%d\n", arr_sta_idx);
        for (int arrival_time = 20 * 60 * 60; arrival_time > 8 * 60 * 60; arrival_time -= 60) {
            struct journey journeys[MAX_NR_STATIONS];
            memset(&journeys, 0, MAX_NR_STATIONS * sizeof(struct journey));
            for (int idx = 0; idx < MAX_NR_STATIONS; ++idx) journeys[idx].route_id = -1;
            journeys[arr_sta_idx].dep_time = arrival_time;

            for (int i = 0; i < MAX_NR_STATIONS; ++i) {
                int next_station = get_latest_idx(journeys);
                struct journey * journey = &journeys[next_station];
                journey->done = true;
                update_journeys(journeys, &network[next_station], journey);
            }

            for (int journey_idx = 0; journey_idx < MAX_NR_STATIONS; ++journey_idx) {
                int transfer_time = journeys[journey_idx].transfer_time;
                if (transfer_time > 0) {
                    total_transfer_sum += transfer_time;
                    num_total_transfer++;
                }
            }
        }
    }
    return (float)total_transfer_sum / (float)num_total_transfer;
}

int main(int argc, char **argv)
{
    int rank, num_procs;
    char offset_buf[4096];
    char result_buf[64];

    srand(time(NULL));
    struct route * routes = read_routes();
    struct station * stations = read_stations();
    struct schedule * schedules = read_schedules();

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    struct schedule * schedule = &schedules[rank];
    struct station * network = build_network(routes, stations, schedule->offsets);
    schedule->efficiency = calculate_network_efficiency(network);
    free(network);
    printf("Proc %d calculated %f\n", rank, schedule->efficiency);

    MPI_Finalize();
    return 0;
}