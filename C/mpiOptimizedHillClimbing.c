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
#define MAX_NR_STATION_WALKS 20
#define MAX_NR_TRIP_STOPS 38
#define MAX_NR_TRIPS 509
#define MAX_NR_ROUTES 495
#define MAX_NR_STATIONS 500
#define MAX_NR_NETWORKS 256

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
    struct schedule * next;
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

void insert_schedule(struct schedule** head, struct schedule* schedule)
{
    if (*head == NULL || (*head)->efficiency >= schedule->efficiency){
        schedule->next = *head;
        *head = schedule;
    }
    else {
        struct schedule* current = *head;
        while (current->next != NULL && current->next->efficiency < schedule->efficiency)
            current = current->next;

        schedule->next = current->next;
        current->next = schedule;
    }
}

struct schedule * read_schedules(struct route * routes) {
    struct schedule * schedules = NULL;
    FILE * stream = fopen("schedules.txt", "r");
    if (!stream) return calloc(1, sizeof(struct schedule));
    char line[4096];
    while(fgets(line, 4096, stream)) {
        struct schedule * schedule = calloc(1, sizeof(struct schedule));
        schedule->efficiency = atof(strtok(line, "|"));
        for (int offset_idx = 0; offset_idx < MAX_NR_ROUTES; ++offset_idx)
            schedule->offsets[offset_idx] = atoi(strtok(NULL, "|"));
        schedule->visited = atoi(strtok(NULL, "\n"));
        insert_schedule(&schedules, schedule);
    }
    fclose(stream);
    return schedules;
}

void write_schedules(struct schedule * schedules) {
    FILE* stream = fopen("schedules.txt", "w");
    struct schedule * schedule = schedules;
    while (schedule != NULL) {
        fprintf(stream, "%f|", schedule->efficiency);
        for (int offset = 0; offset < MAX_NR_ROUTES; ++offset) fprintf(stream, "%d|", schedule->offsets[offset]);
        fprintf(stream, "%d", schedule->visited);
        if (schedule->next) fprintf(stream, "\n");
        schedule = schedule->next;
    }
    fclose(stream);
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

void update_offsets(int * dest_offsets, int * src_offsets, struct route routes[], int schedule_idx) {
    int route_idx = schedule_idx / 2;
    int max_headway = routes[route_idx].max_headway;
    int offset = schedule_idx % 2 ? -1 : 1;
    memcpy(dest_offsets, src_offsets, MAX_NR_ROUTES * sizeof(int));
    dest_offsets[route_idx] += offset;
    if (dest_offsets[route_idx] < -max_headway) dest_offsets[route_idx] = -max_headway;
    if (dest_offsets[route_idx] >  max_headway) dest_offsets[route_idx] =  max_headway;
}

int get_unique_schedule_idx(const int schedule_idxs[], int limit) {
    while (true) {
        bool duplicate = false;
        int schedule_idx = rand() % (2*MAX_NR_ROUTES);
        for (int idx = 0; idx < limit; ++idx) duplicate |= schedule_idx == schedule_idxs[idx];
        if (!duplicate) return schedule_idx;
    }
}

int main(int argc, char **argv)
{
    int rank, num_procs;
    char offset_buf[4096];
    char result_buf[64];

    srand(time(NULL));
    struct route * routes = read_routes();
    struct station * stations = read_stations();

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (rank == 0) {
        struct schedule * list = read_schedules(routes);
        if (list->efficiency == 0) {
            struct station * network = build_network(routes, stations, list->offsets);
            list->efficiency = calculate_network_efficiency(network);
            free(network);
            write_schedules(list);
        }

        int epoch = 183;
        while (true) {
            clock_t start = clock();
            struct schedule * schedules = calloc(MAX_NR_NETWORKS, sizeof(struct schedule));
            struct schedule * first = list;
            while (first && first->visited) first = first->next;
            if (!first) break;
            printf("Epoch %d: min eff = %f\n", epoch++, first->efficiency);
            fflush(stdout);

            int schedule_idxs[MAX_NR_NETWORKS];
            for (int calc_idx = 0; calc_idx < MAX_NR_NETWORKS; calc_idx++) {
                int schedule_idx = get_unique_schedule_idx(schedule_idxs, calc_idx);
                schedule_idxs[calc_idx] = schedule_idx;

                int * offsets = schedules[calc_idx].offsets;
                update_offsets(offsets, first->offsets, routes, schedule_idx);
                int proc_id = num_procs - 1 - (calc_idx % num_procs);
                if (proc_id) {
                    int size = 0;
                    for (int idx = 0; idx < MAX_NR_ROUTES; idx++) size += sprintf(&offset_buf[size], "%d|", offsets[idx]);
                    MPI_Send(offset_buf, sizeof(offset_buf), MPI_CHAR, proc_id, 0, MPI_COMM_WORLD);
                }
                else {
                    struct schedule * schedule = &schedules[calc_idx];
                    struct station * network = build_network(routes, stations, offsets);
                    schedule->efficiency = calculate_network_efficiency(network);
                    free(network);
                }
            }

            for (int calc_idx = 0; calc_idx < MAX_NR_NETWORKS; calc_idx++) {
                int proc_id = num_procs - 1 - (calc_idx % num_procs);
                if (proc_id) {
                    MPI_Recv(result_buf, sizeof(result_buf), MPI_CHAR, proc_id, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    struct schedule * schedule = &schedules[calc_idx];
                    schedule->efficiency = atof(strtok(result_buf, "|"));
                }
            }

            for (int calc_idx = 0; calc_idx < MAX_NR_NETWORKS; calc_idx++) {
                struct schedule * schedule = &schedules[calc_idx];
                if (schedule->efficiency < first->efficiency) {
                    bool seen = false;
                    struct schedule * node = list;
                    while (node != NULL) {
                        bool same = true;
                        for (int offset = 0; offset < MAX_NR_ROUTES; ++offset) {
                            if (schedule->offsets[offset] != node->offsets[offset]) same = false;
                        }
                        seen |= same;
                        node = node->next;
                    }
                    if (!seen) printf("new sched found: %f\n", schedule->efficiency);
                    if (!seen) insert_schedule(&list, schedule);
                }
            }
            clock_t end = clock();
            printf("entire %ld micro seconds\n", end - start);
            fflush(stdout);
            write_schedules(list);
            first->visited = true;
        }

        MPI_Abort(MPI_COMM_WORLD, 0);
    }
    else {
        while (true) {
            MPI_Recv(offset_buf, sizeof(offset_buf), MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            int recv_offset[MAX_NR_ROUTES];
            for (int route_id = 0; route_id < MAX_NR_ROUTES; route_id++) {
                if (route_id == 0) recv_offset[route_id] = atoi(strtok(offset_buf, "|"));
                else recv_offset[route_id] = atoi(strtok(NULL, "|"));
            }
            clock_t start = clock();
            struct station * network = build_network(routes, stations, recv_offset);
            float efficiency = calculate_network_efficiency(network);
            free(network);
            if (rank == 1) {
                clock_t end = clock();
                printf("proc %ld micro seconds\n", end - start);
                fflush(stdout);
            }
            sprintf(result_buf, "%f|", efficiency);
            MPI_Send(result_buf, sizeof(result_buf), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
