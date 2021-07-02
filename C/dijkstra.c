#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

#define MAX_NR_STATION_STOPS 554
#define MAX_NR_STATION_WALKS 17
#define MAX_NR_TRIP_STOPS 31
#define MAX_NR_TRIPS 525
#define MAX_NR_ROUTES 10
#define MAX_NR_STATIONS 221
#define MAX_NR_THREADS 2*MAX_NR_ROUTES

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
    FILE * stream = fopen("../routes.txt", "r");
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
    FILE * stream = fopen("../stations.txt", "r");
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
    FILE * stream = fopen("../schedules.txt", "r");
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

    for (int route_id = 0; route_id < MAX_NR_ROUTES; ++route_id) {
        int max_headway_mts = routes[route_id].max_headway / 60;
        schedules->offsets[route_id] = (rand() % (2 * max_headway_mts + 1)) - max_headway_mts;
    }
    return schedules;
}

void write_schedules(struct schedule * schedules) {
    FILE* stream = fopen("../schedules.txt", "w");
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
                journeys[next_station].done = true;
                update_journeys(journeys, &network[next_station], &journeys[next_station]);
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

struct route * routes;
struct station * stations;

void * thread_func(void * t_schedule)
{
    struct schedule * schedule = (struct schedule *)t_schedule;
    struct station * network = build_network(routes, stations, schedule->offsets);
    schedule->efficiency = calculate_network_efficiency(network);
    free(network);
    pthread_exit(NULL);
}

void start_thread(pthread_t * thread, struct schedule * schedule, int route_idx, int max_headway, int offset) {
    int * offsets = schedule->offsets;
    offsets[route_idx] += offset;
    if (offsets[route_idx] < -max_headway) offsets[route_idx] = -max_headway;
    if (offsets[route_idx] >  max_headway) offsets[route_idx] =  max_headway;
    int rc = pthread_create(thread, NULL, thread_func, (void *)schedule);
    if (rc) {
        printf("Error: unable to create thread %d, %d\n", route_idx, rc);
        exit(-1);
    }
}

int main()
{
    srand(time(NULL));
    routes = read_routes();
    stations = read_stations();
    struct schedule * list = read_schedules(routes);
    if (list->efficiency == 0) {
        struct station * network = build_network(routes, stations, list->offsets);
        list->efficiency = calculate_network_efficiency(network);
        free(network);
    }

    for (int idx = 0; idx < 10; ++idx) {
        pthread_t threads[MAX_NR_THREADS];
        struct schedule * schedules = calloc(MAX_NR_THREADS, sizeof(struct schedule));
        struct schedule * first = list;
        while (first && first->visited) first = first->next;
        if (!first) break;
        first->visited = true;
        printf("Epoch %d: first eff = %f\n", idx, first->efficiency);
        fflush(stdout);

        for (int thread_idx = 0; thread_idx < MAX_NR_THREADS; ++thread_idx) {
            int route_idx = thread_idx / 2;
            int max_headway = routes[route_idx].max_headway;
            int offset = thread_idx % 2 ? -1 : 1;
            memcpy(&schedules[thread_idx].offsets, &first->offsets, MAX_NR_ROUTES*sizeof(int));
            start_thread(&threads[thread_idx], &schedules[thread_idx], route_idx, max_headway, offset);
        }

        for (int thread_idx = 0; thread_idx < MAX_NR_THREADS; ++thread_idx) {
            pthread_join(threads[thread_idx], NULL);
            struct schedule * schedule = &schedules[thread_idx];
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
                if (!seen) insert_schedule(&list, schedule);
            }
        }

        write_schedules(list);
    }
}