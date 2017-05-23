//Rahul Jain rj8656 Leo Xia lx939
using namespace std;
#include <iostream>
#include <mpi.h>
#include <fstream>
#include <vector>
#include <cstdlib>

int num_row=-1;
int num_col=-1;
int* mat_buffer;
int* vec_buffer;

void inputMatrix(std::vector<int> &v){
    //INPUT MATRIX FILE
    std::fstream myfile("matrix-1.txt", std::ios_base::in);
    
    int val;
    bool got_row_count = false;
    
    while (myfile >> val){
        if (got_row_count == false){
            got_row_count = true;
            num_row = val;
            std::cout << "MATRIX: ";
        }else{
            v.push_back(val);
            std::cout << val << " ";
        }
    }
    //for every value in vector put it into a buffer
    mat_buffer = (int*) malloc(sizeof(int) * v.size());
    for(int i=0; i < v.size(); i++) {
        mat_buffer[i] = v[i];
    }
    
}

void inputVector(std::vector<int> &v2){
    //INPUT VECTOR FILE
    std::fstream myfile2("vector-1.txt", std::ios_base::in);
    
    int val2;
    int count = 0;
    std::cout << "VECTOR: ";
    while (myfile2 >> val2){
        v2.push_back(val2);
        count++;
        std::cout << val2 << " ";
    }
    num_col = count;
    vec_buffer = (int*) malloc(sizeof(int) * v2.size());
    for(int i=0; i< v2.size(); i++){
        vec_buffer[i] = v2[i];
    }
    
    
}


int main(void) {
    int rank, size;
    MPI_Init(NULL, NULL);
    
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);
    
    
    //int num_col;
    //int num_row;
    std::vector<int> mat;
    std::vector<int> vec;
    
    int* product;
    int* proc_mat;
    int* proc_prod;
    int* mat_sendcounts = (int*) malloc(sizeof(int) * size);
    
    
    int num_rows_sent;
    int index =0;
    if(rank == 0) {
        
        inputMatrix(mat);
        inputVector(vec);
        
    }
    //broadcast column #
    MPI_Bcast(&num_col, 1, MPI_INT,0,MPI_COMM_WORLD);
    if (rank == 0 ){
        //calculation of mat_send counts which tells program how to divide up the rows for the processes
        int mat_elements_per_proc = num_row/size;
        int leftover_mat = num_row - mat_elements_per_proc*(size - 1);
        for(int i=0; i<size-1; i++){
            mat_sendcounts[i] = mat_elements_per_proc;
            //cout << "rows: " << mat_sendcounts[i];
        }
        mat_sendcounts[size-1] = leftover_mat;
        
        product = (int*) malloc(sizeof(int) * num_col);
        for(int i = 1; i < size; i++) {
            num_rows_sent = mat_sendcounts[i-1];
            index += num_rows_sent;
            MPI_Send(&index, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&num_rows_sent, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&mat_buffer[index*num_col], num_rows_sent*num_col, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        //for process 0
        num_rows_sent = mat_sendcounts[0];
        index = 0;
        proc_mat = mat_buffer;
        proc_prod = product;
    } else {
        //recieve stuff here
        MPI_Recv(&index, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&num_rows_sent, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        proc_mat = (int*) malloc(sizeof(int) * num_rows_sent*num_col);
        vec_buffer = (int*) malloc(sizeof(int) * num_col);
        MPI_Recv(proc_mat, num_rows_sent*num_col, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        proc_prod = (int*) malloc(sizeof(int) * num_rows_sent);
    }
    //MPI BROADCAST for Vector
    MPI_Bcast(vec_buffer, num_col, MPI_INT, 0, MPI_COMM_WORLD);
    
    //calculate the dot product here
    for(int i = 0; i < num_rows_sent; i++) {
        proc_prod[i] = 0;
        for(int j = 0; j < num_col; j++) {
            proc_prod[i] += vec_buffer[j]*proc_mat[i*num_col+j];
        }
    }
    
    
    if(rank != 0) {
        //send results
        MPI_Send(&index, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&num_rows_sent, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(proc_prod, num_rows_sent, MPI_INT, 0, 0, MPI_COMM_WORLD);
    } else {
        //gather
        for(int i = 1; i < size; i++) {
            MPI_Recv(&index, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&num_rows_sent, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&product[index], num_rows_sent, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        ofstream finalResult;
        finalResult.open("result.txt");
        for(int i = 0; i < num_row; i++) {
            finalResult << product[i] << " ";
        }
        finalResult << endl;
        finalResult.close();
    }
    
    
    MPI_Finalize();
    return 0;
}