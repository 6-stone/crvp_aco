/*********************************
 
 Ant Colony Optimization algorithms (AS, ACS, EAS, RAS, MMAS, BWAS) for CVRP
 
 Created by 孙晓奇 on 2016/10/8.
 Copyright © 2016年 xiaoqi.sxq. All rights reserved.
 
 Program's name: acovrp
 Purpose: local search routines
 
 email: sunxq1991@gmail.com
 
 *********************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include "ls.h"
#include "InOut.h"
#include "vrp.h"
#include "ants.h"
#include "utilities.h"

long int ls_flag;          /* indicates whether and which local search is used */ 
long int nn_ls;            /* maximal depth of nearest neighbour lists used in the 
                              local search */
long int dlb_flag = TRUE;  /* flag indicating whether don't look bits are used. I recommend 
			                  to always use it if local search is applied */
/*
 FUNCTION:       generate a random permutation of the integers 0 .. n-1
 INPUT:          length of the array
 OUTPUT:         pointer to the random permutation
 (SIDE)EFFECTS:  the array holding the random permutation is allocated in this
 function. Don't forget to free again the memory!
 COMMENTS:       only needed by the local search procedures
 */
long int * generate_random_permutation( long int n )
{
   long int  i, help, node, tot_assigned = 0;
   double    rnd;
   long int  *r;

   r = malloc(n * sizeof(long int));  

   for ( i = 0 ; i < n; i++) 
     r[i] = i;

   for ( i = 0 ; i < n ; i++ ) {
     /* find (randomly) an index for a free unit */ 
     rnd  = ran01 ( &seed );
     node = (long int) (rnd  * (n - tot_assigned)); 
     assert( i + node < n );
     help = r[i];
     r[i] = r[i+node];
     r[i+node] = help;
     tot_assigned++;
   }
   return r;
}

/*
 FUNCTION:       2-opt all routes of an ant's solution.
 INPUT:          tour, an ant's solution
                 depotId
 OUTPUT:         none
 */
void two_opt_solution(long int *tour, long int tour_size)
{
    long int *dlb;               /* vector containing don't look bits */
    long int *route_node_map;    /* mark for all nodes in a single route */
    long int *tour_node_pos;     /* positions of nodes in tour */

    long int route_beg = 0;
    
    dlb = malloc(num_node * sizeof(long int));
    for (int i = 0 ; i < num_node; i++) {
        dlb[i] = FALSE;
    }
    
    route_node_map =  malloc(num_node * sizeof(long int));
    for (int j = 0; j < num_node; j++) {
        route_node_map[j] = FALSE;
    }
    
    tour_node_pos =  malloc(num_node * sizeof(long int));
    for (int j = 0; j < tour_size; j++) {
        tour_node_pos[tour[j]] = j;
    }
    
    for (int i = 1; i < tour_size; i++) {
        // 2-opt a single route from tour
        if (tour[i] == 0) {
            tour_node_pos[0] = route_beg;
            two_opt_single_route(tour, route_beg, i-1, dlb, route_node_map, tour_node_pos);
            
            for (int j = 0; j < num_node; j++) {
                route_node_map[j] = FALSE;
            }
            route_beg = i;
        } else {
            route_node_map[tour[i]] = TRUE;
        }
    }
    
    
    free( dlb );
    free(route_node_map);
    free(tour_node_pos);
}

/*
 FUNCTION:       2-opt a single route from an ant's solution.
                 This heuristic is applied separately to each
 of the vehicle routes built by an ant.
 INPUT:          rbeg, route的起始位置
                 rend, route的结束位置(包含rend处的点)
 OUTPUT:         none
 COMMENTS:       the neighbourhood is scanned in random order
 */
void two_opt_single_route(long int *tour, long int rbeg, long int rend,
                          long int *dlb, long int *route_node_map, long int *tour_node_pos)
{
    long int n1, n2;                            /* nodes considered for an exchange */
    long int s_n1, s_n2;                        /* successor nodes of n1 and n2     */
    long int p_n1, p_n2;                        /* predecessor nodes of n1 and n2   */
    long int pos_n1, pos_n2;                    /* positions of nodes n1, n2        */
    long int num_route_node = rend - rbeg + 1;  /* number of nodes in a single route(depot只计一次) */
    
    long int i, j, h, l;
    long int improvement_flag, help, n_improves = 0, n_exchanges = 0;
    long int h1=0, h2=0, h3=0, h4=0;
    long int radius;             /* radius of nn-search */
    long int gain = 0;
    long int *random_vector;

    // debug
//    print_single_route(0, tour + rbeg, num_route_node+1);

    improvement_flag = TRUE;
    random_vector = generate_random_permutation(num_route_node);

    while ( improvement_flag ) {

        improvement_flag = FALSE;

        for (l = 0 ; l < num_route_node; l++) {

            /* the neighbourhood is scanned in random order */
            pos_n1 = rbeg + random_vector[l];
            n1 = tour[pos_n1];
            if (dlb_flag && dlb[n1])
                continue;
            
            s_n1 = pos_n1 == rend ? tour[rbeg] : tour[pos_n1+1];
            radius = instance.distance[n1][s_n1];
            /* First search for c1's nearest neighbours, use successor of c1 */
            for ( h = 0 ; h < nn_ls ; h++ ) {
                n2 = instance.nn_list[n1][h]; /* exchange partner, determine its position */
                if (route_node_map[n2] == FALSE) {
                    /* 该点不在本route中 */
                    continue;
                }
                if (radius > instance.distance[n1][n2] ) {
                    pos_n2 = tour_node_pos[n2];
                    s_n2 = pos_n2 == rend ? tour[rbeg] : tour[pos_n2+1];
                    gain =  - radius + instance.distance[n1][n2] +
                            instance.distance[s_n1][s_n2] - instance.distance[n2][s_n2];
                    if ( gain < 0 ) {
                        h1 = n1; h2 = s_n1; h3 = n2; h4 = s_n2;
                        goto exchange2opt;
                    }
                }
                else break;
            }
            
            /* Search one for next c1's h-nearest neighbours, use predecessor c1 */
            p_n1 = pos_n1 == rbeg ? tour[rend] : tour[pos_n1-1];
            radius = instance.distance[p_n1][n1];
            for ( h = 0 ; h < nn_ls ; h++ ) {
                n2 = instance.nn_list[n1][h];  /* exchange partner, determine its position */
                if (route_node_map[n2] == FALSE) {
                    /* 该点不在本route中 */
                    continue;
                }
                if ( radius > instance.distance[n1][n2] ) {
                    pos_n2 = tour_node_pos[n2];
                    p_n2 = pos_n2 == rbeg ? tour[rend] : tour[pos_n2-1];
                    
                    if ( p_n2 == n1 || p_n1 == n2)
                        continue;
                    gain =  - radius + instance.distance[n1][n2] +
                            instance.distance[p_n1][p_n2] - instance.distance[p_n2][n2];
                    if ( gain < 0 ) {
                        h1 = p_n1; h2 = n1; h3 = p_n2; h4 = n2;
                        goto exchange2opt;
                    }
                }
                else break;
            }
            /* No exchange */
            dlb[n1] = TRUE;
            continue;

exchange2opt:
            n_exchanges++;
            improvement_flag = TRUE;
            dlb[h1] = FALSE; dlb[h2] = FALSE;
            dlb[h3] = FALSE; dlb[h4] = FALSE;
            /* Now perform move */
            if ( tour_node_pos[h3] < tour_node_pos[h1] ) {
                help = h1; h1 = h3; h3 = help;
                help = h2; h2 = h4; h4 = help;
            }
            /* reverse inner part from pos[h2] to pos[h3] */
            i = tour_node_pos[h2]; j = tour_node_pos[h3];
            while (i < j) {
                n1 = tour[i];
                n2 = tour[j];
                tour[i] = n2;
                tour[j] = n1;
                tour_node_pos[n1] = j;
                tour_node_pos[n2] = i;
                i++; j--;
            }
            // debug
//            print_single_route(0, tour + rbeg, num_route_node+1);
        }
        if ( improvement_flag ) {
            n_improves++;
        }
    }
    free( random_vector );
}
