#include<cstdlib>
#include<stdlib.h>
#include<iostream>
#include<chrono>
#include<fstream>
#include<ctime>
#include<vector>
#include<fstream>
#include<string>
#include<sstream>
#include<chrono>
#include<cmath>
#include<vector>
#include<random>
#include<thread>
#include<time.h>
#include "City.h"
#include "ParallelFunctions.h"

//------------------------------------------------------------------------------
//CONSTANTS
const double ALPHA = 2 ;
const double BETA = 5 ;
const double RO = 0.7 ;
const double Q = 1 ;
const int ANTS = 256;


//==============================================================================
//MAIN FUNCTION
//------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
	if(argc < 4){
			std::cerr<<'\n'<<"Enter the correct inputs please !";
			std::cerr<<'\n'<<"<fileName> <i/p file.txt> <no_of_Iterations> <Parallelism Degree>"<<std::endl;
			return -1;
		}
	
    //194.txt
    std::string file_Name = argv[1];
    int no_Of_Iterations = atoi(argv[2]);
    int p_d = atoi(argv[3]) ;
    
    if( p_d > ANTS ){
			std::cerr<<'\n'<<"The parallelism degree is higher than no. of Ants!!!"<<std::endl;
			return -1;
			
		}
    
    std::vector<City> cities = generateCities(file_Name);
    unsigned int noc = cities.size();
    std::vector<std::vector<double>> A = generateDistanceMatrix( cities );     // generate distance matrix
   
// INITIALIZATION OF PARAMETERS

    double Tau = 1; // initial value for Tau
    std::vector < std::vector < double > > Tau_Matrix( noc , std::vector<double>(noc) );
    updateTauMatrix( Tau_Matrix , Tau );
    std::vector<std::vector<City>> Tour(ANTS);     
    std::pair<double , std::vector<City> > Global_Best;
    std::vector<std::pair<double , std::vector<City>>> TourLength(ANTS);
   

 //=============================================================================
//==============================================================================
    //***************** MAIN ITERATION *****************************************
//==============================================================================
auto start = std::chrono::high_resolution_clock::now();
	bool first = true;
    std::vector<int> tiles = tiling_jobs( p_d , ANTS );
    /*
    for(int v = 0 ; v  < tiles.size();v++){
			std::cout<<  "tiles are = " << tiles[v] << std::endl;
		}
    */
    for ( int i = 0 ; i < no_Of_Iterations ; i++ ) { // for each Iteration
		
		std::vector<std::thread> workers;
//------------------------------------------------------------------------ Finding Tours in Parallel
		
        for(int it = 0 ; it < tiles.size() - 1; it++){		
            workers.push_back(std::thread(finding_paths, tiles[it],tiles[it+1], std::ref(Tour) ,std::ref(Tau_Matrix) ,std::ref(A) ,std::ref(cities) ));
            
        }
        for(unsigned t = 0 ; t  < workers.size(); t++){
				workers[t].join();
		}
		
//--------------------------------------------------------------------------- Updating phermone on edges
		Update_Phermones( Tour , TourLength, A ,Tau_Matrix , Q);
//------------------------------------------------------------------------------ Find the best Tour
		
			std::pair< double , std::vector<City> > best = find_best(TourLength);
			
			if(first){
					Global_Best = best;
					first = false;
				}
				
			else if(best.first < Global_Best.first){
					Global_Best = best;
				}
				
				
			updateTauMatrix2(Tau_Matrix , 1 - RO );	// Updating the Tau matrix by RO ( Evaporation Rate )
			
		

    } // End of Iterations

auto end = std::chrono::high_resolution_clock::now() - start;
auto exeTimea = std::chrono::duration_cast<std::chrono::milliseconds>(end).count();
std::cout << "The execution time is = (" << exeTimea << ") Milliseconds " <<std::endl;
std::cout << "The Best Tour is  : "<< std::endl;
for(int i = 0 ; i < Global_Best.second.size() ; i++) {
		std::cout<<" -> "<< Global_Best.second[i].getId() ;
	} 
std::cout<<std::endl;
std::cout<<"with the length of : "<< Global_Best.first << std::endl ;
//==============================================================================
    //***************** END OF THE MAIN ITERATION ******************************
//==============================================================================
//==============================================================================
return 0;
} // End of main()

// FUNCTIONS DEFINITIONS
//==============================================================================
// generateCities function
std::vector<City> generateCities(std::string fileDirectory){
    std::vector<City> cities;
    std::ifstream f(fileDirectory);
    if(f.is_open()){
        while(f.good()){ // Reading file line by line
            int id1;
            f >> id1;
            double latitude1;
            f >> latitude1;
            double longitude1;
            f >> longitude1;
            City city( id1 , latitude1 , longitude1 );
            cities.push_back(city);
        }
        f.close();
        return cities;
    }
    else{
        std::cout<<"ERROR : File Open !!! "<<std::endl;
        return std::vector<City>(); // Return an empty vector if the file fails to open
    }
}// End of generateCities

//------------------------------------------------------------------------------
//cal_distance Function for calculating the distance between two cities.
//------------------------------------------------------------------------------
double cal_distance ( City city1 , City city2 ){
   return  (double) std::sqrt( pow(city1.getLatitude() - city2.getLatitude() , 2) +  pow(city1.getLongitude() - city2.getLongitude() , 2 ));
}

//------------------------------------------------------------------------------
//generateDistanceMatrix Function
//------------------------------------------------------------------------------
std::vector<std::vector<double>> generateDistanceMatrix(const std::vector<City>& cities){
    unsigned int noc = cities.size();
    std::vector<std::vector<double>> A;

    for(unsigned int i = 0 ; i < noc ; i++ ){
        std::vector<double> row;
        for(unsigned int j = 0 ; j < noc; j++ ){
            row.push_back(cal_distance( cities[i] , cities[j] ));
            }
        A.push_back(row);
    }
return A ;
}

//------------------------------------------------------------------------------
// print distance matrix
//------------------------------------------------------------------------------
void printDistanceVector(std::vector<std::vector<double>>& A){
    unsigned int noc = A[0].size();
    for(unsigned int i = 0 ; i < noc ; i++){
        std::cout<<"["<<"\t";
        for(unsigned int j = 0 ; j < noc ; j++){
            std::cout << A[i][j] << "\t" ;
        }
        std::cout<<"]"<<std::endl;
    }
}

//------------------------------------------------------------------------------
// updateTauMatrix (Local)
//------------------------------------------------------------------------------
void updateTauMatrix(std::vector<std::vector<double>>& t,double Tau){
    unsigned int size = t[0].size();
    for(unsigned int i = 0 ; i < size ; i++){
        for(unsigned int j = 0 ; j < size ; j++){
            if( i == j ){
                t[i][j] = 0 ;
            }
            else{
                t[i][j] = t[i][j] + Tau ;
            }
        }
    }
}
//------------------------------------------------------------------------------
// updateTauMatrix (Global)
//------------------------------------------------------------------------------
void updateTauMatrix2( std::vector<std::vector<double>>& t , double RO ){
    unsigned int size = t[0].size();
    for(unsigned int i = 0 ; i < size ; i++){
        for(unsigned int j = 0 ; j < size ; j++){
            if( i == j ){
                t[i][j] = 0 ;
            }
            else{
                t[i][j] = t[i][j] * RO ;
            }
        }
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::vector<City> findPath(std::vector<std::vector<double>>& Tau_Matrix, std::vector<std::vector<double>>& A, std::vector<City>& cities){
     // constructing visited and allowed vectors for ant 'a'
            std::vector<City> visited;
            std::vector<City> allowed;
            std::random_device sd;
            std::mt19937 gene(sd());
            std::uniform_real_distribution<> diss( 0 , cities.size() - 1);
            visited.push_back( cities[ diss(gene) ] );
            for ( int c = 0 ; c < cities.size() ; c++ ) {
                if( cities[c].getId() != visited[0].getId() ) {
                    allowed.push_back( cities[c] );
                }
            }

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis( 0 , 1);
            int s = 0 ; // Keeping track of last visited city
            while( !allowed.empty() ){ // Until we visit the last city          // if we visit the last city in allowed for next city
                if(allowed.size() == 1 ){ // Last city to visit
                    visited.push_back(allowed[0]);
                    allowed.erase(allowed.begin());
                    break;
                }

                double sum = 0 ;
                double summation;
                for( int ss = 0 ; ss < allowed.size() ; ss++ ) { // Calculating Denominator for probability
                    summation =  pow( Tau_Matrix[visited[s].getId() - 1 ][allowed[ss].getId() - 1] , ALPHA ) * pow( ( 1 / ( A[visited[s].getId() - 1][allowed[ss].getId() - 1] ) ) , BETA );
                    sum = sum + summation ;
                }
                double randi = dis(gen);
                double summi = 0 ;
                double probability_value ;
                for (int p = 0 ; p < allowed.size() ; p++){
                    probability_value = (pow(Tau_Matrix[visited[s].getId() - 1][allowed[p].getId() - 1] , ALPHA) * pow(( 1 / A[visited[s].getId()- 1][allowed[p].getId() - 1]), BETA) ) / sum ;
                    summi += probability_value;        //Roulette Wheel Selection for Next city to visit
                    if( randi <= summi ){
                        visited.push_back(allowed[p]);
                        allowed.erase( allowed.begin() + p );
                        break;
                    }
                }
            s++;

            } // End of " While Loop " . Tour of the ant is completed

            /*
            std::cout <<"The Tour is :"<< std::endl ;
            for(int i = 0 ; i < visited.size();i++){
                std::cout << "-> "<<visited[i].getId();
            }
            * */
            return visited;
}

void updateEdges(std::vector<std::vector<double>>& Tau_Matrix, std::vector<City>& visited, double& deltaTau){
    
             for( int visit = 0  ; visit < visited.size() ; visit++ ){             // updating Taues
                if( visit == visited.size() - 1 ){
                    Tau_Matrix[visited[visited.size()-1].getId() - 1][visited[0].getId()-1] +=  deltaTau ;
                    Tau_Matrix[visited[0].getId()-1][visited[visited.size()-1].getId()-1] +=  deltaTau ;
                    break;
                }
                    Tau_Matrix[(visited[visit].getId()) - 1][(visited[visit + 1].getId()) - 1] +=   deltaTau ;
                    Tau_Matrix[(visited[visit + 1].getId()) - 1][(visited[visit].getId()) - 1] +=   deltaTau ;
            }

}

void finding_paths(int first , int last , std::vector<std::vector<City>>& Tours,std::vector<std::vector<double>>& Tau_Matrix, std::vector<std::vector<double>>& A, std::vector<City>& cities ){

    for(int i = first ; i < last ; i++){

        Tours[i] = findPath(Tau_Matrix , A , cities );
         
    }

}

std::vector<int> tiling_jobs(int& nw , int ants ){
    std::vector<int> tiles;
    tiles.push_back(0);
    
    if(nw == 1){
        tiles.push_back(ants);
        return tiles;
    }
    else if(nw <= ants){
        int next = 0;
        int n = ants;
        
        for(int j = 0 ; j < nw ; j++){
            int dis = ceil((double) n / ( nw - j ));
            next += dis;
            tiles.push_back(next);
            n = n - dis;
        }
        return tiles;
    }
    else{
        std::cout << "Parallelism degree is greater than the no. of Ants !" << std::endl;
        return std::vector<int>(); // Return an empty vector if the conditions are not met
    }
}

std::pair<double , std::vector<City>> find_best (std::vector<std::pair<double , std::vector<City>>>& TourLength){
		std::pair<double , std::vector<City>> best = TourLength[0] ;
		for(int i =  1 ; i < TourLength.size() ; i++){
				if(TourLength[i].first < best.first ){
						best = TourLength[i];	
					}				
			}
		return best;
}

void Update_Phermones(std::vector<std::vector<City>>& Tours ,std::vector<std::pair<double , std::vector<City>>>& TourLength,std::vector<std::vector<double>>& A, std::vector<std::vector<double>>& Tau_Matrix , double Q){
		for(int t = 0 ; t < Tours.size() ; t++ ){
			
			
				double L = 0;
			    for(int vl = 0 ; vl+1 < Tours[t].size() ; vl ++ ){
						L = L + A[Tours[t][vl].getId() - 1][Tours[t][vl + 1].getId()-1];
					}
					L = L + A[Tours[t][0].getId()-1][Tours[t][Tours[t].size()-1].getId()-1];
					
				
				TourLength[t] = make_pair(L,Tours[t]);
					
				double deltaTau = Q / L;
				
				updateEdges(Tau_Matrix, Tours[t],deltaTau);
				
			}
	}
