#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>

struct Matrix {
    int n,m;
    std::vector<std::vector<int>> matr;
    Matrix(int n=0,int m=0):n(n),m(m){
        matr.resize(n);
        for (int i=0;i<n;++i) matr[i].resize(m);
        matr.assign(n,std::vector<int>(m,0));
    }

    Matrix(const std::vector<std::vector<int>> &vect) {
        n = vect.size();
        m = vect[0].size();
        matr = vect;
    }

    Matrix& operator=(const Matrix& x) {
        this->n=x.n;
        this->m=x.m;
        this->matr=x.matr;
        return *this;
    }

    Matrix& operator+=(const Matrix&rhs) {
        if (n!=rhs.n||m!=rhs.m) throw std::runtime_error("Can't sum matrix with different dimensions");
        for (int i=0;i<n;++i)
            for (int j=0;j<m;++j) matr[i][j]+=rhs.matr[i][j];
        return *this;
    }
};

std::ostream& operator<<(std::ostream& os,const Matrix& matrix) {
    for (auto& x:matrix.matr) {
        for (auto a:x) os<<a<<' ';
        os<<'\n';
    }
    return os;
}

Matrix operator+(const Matrix& lhs,const Matrix& rhs){
    if (lhs.n!=rhs.n||lhs.m!=rhs.m) throw std::runtime_error("Can't sum matrix with different dimensions");
    Matrix ans = Matrix(lhs.n,lhs.m);
    for (int i=0;i<lhs.n;++i)
        for (int j=0;j<lhs.m;++j) ans.matr[i][j]=lhs.matr[i][j]+rhs.matr[i][j];
    return ans;
}

struct MatrixOfMatrix {
    int n,m,elemN,elemM;
    std::vector<std::vector<Matrix>> matr;
    MatrixOfMatrix(int n,int m,int N,int M):n(n),m(m),elemN(N),elemM(M) {
        matr.resize(n);
        for (int i=0;i<n;++i) matr[i].resize(m);
        for (int i=0;i<n;++i)
            for (int j=0;j<m;++j) matr[i][j]=Matrix(elemN,elemM);
    }

    Matrix cast() {
        Matrix ans = Matrix(elemN*n,elemM*m);
        for (int i=0;i<n;++i)
            for (int j=0;j<m;++j)
                for (int I=0;I<elemN;++I)
                    for (int J=0;J<elemM;++J) ans.matr[i*elemN+I][j*elemM+J] = matr[i][j].matr[I][J];
        return ans;
    }
};

std::vector<std::vector<int>> SimpleMulitiplication(std::vector<std::vector<int>>&A,
                                                    std::vector<std::vector<int>>&B) {
    if (A.size()==0||B.size()==0) return {};
    std::vector<std::vector<int>>C (A.size(),std::vector<int>(B[0].size(),0));
    for (int i = 0; i < C.size(); ++i)
        for (int j = 0; j < C[0].size(); ++j) {
            C[i][j] = 0;
            for (int k = 0; k < A[0].size(); ++k) C[i][j] += A[i][k] * B[k][j];
        }
    return C;
}

void Multiple(Matrix& res,const Matrix& A,const Matrix& B) {
    Matrix C(A.n,B.m);
    for (int i=0;i<A.n;++i)
        for (int j=0;j<B.m;++j) {
            C.matr[i][j]=0;
            for (int k = 0; k < A.m; ++k) {
                C.matr[i][j] += A.matr[i][k]*B.matr[k][j];
            }
        }
    res+= C;
}

MatrixOfMatrix MultipleBig(const MatrixOfMatrix& A,const MatrixOfMatrix& B,int threadNumber) {
    std::vector<std::thread> threads(threadNumber);
    MatrixOfMatrix ans= MatrixOfMatrix(A.n,B.m,A.elemN,B.elemM);
    int currentThread=0;
    for (int i=0;i<A.n;++i)
        for (int j=0;j<B.m;++j)
            for (int k=0;k<A.m;++k) {
                if (currentThread==threadNumber) {
                    for (auto&x:threads) x.join();
                    currentThread=0;
                }
                threads[currentThread++] = std::thread(Multiple,std::ref(ans.matr[i][j]),std::ref(A.matr[i][k]),std::ref(B.matr[k][j]));
            }
    for (auto&x:threads) x.join();
    return ans;
}

MatrixOfMatrix Split(const Matrix& x,int nSize,int mSize) {
    int numN = x.n/nSize;
    int numM = x.m/mSize;
    MatrixOfMatrix matrix = MatrixOfMatrix(numN,numM,nSize,mSize);
    for (int num1 = 0;num1<numN;++num1)
        for (int num2=0;num2<numM;++num2) {
            Matrix a = Matrix(nSize,mSize);
            for (int i = 0; i < nSize; ++i)
                for (int j = 0; j < mSize; ++j) a.matr[i][j]=x.matr[num1*nSize+i][num2*mSize+j];
            matrix.matr[num1][num2]=a;
        }
    return matrix;
}

Matrix Calc1(const Matrix& A, const Matrix& B,int threadsNumber) {
    MatrixOfMatrix bigA = Split(A,A.n/threadsNumber,A.m);
    MatrixOfMatrix bigB = Split(B,B.n,B.m/threadsNumber);
    return MultipleBig(bigA,bigB,threadsNumber).cast();
}

Matrix Calc2(const Matrix& A, const Matrix& B,int threadsNumber) {
    MatrixOfMatrix bigA = Split(A,A.n,A.m/threadsNumber);
    MatrixOfMatrix bigB = Split(B,B.n/threadsNumber,B.m);
    return MultipleBig(bigA,bigB,threadsNumber).cast();
}

Matrix Calc3(const Matrix& A, const Matrix& B,int threadsNumber) {
    MatrixOfMatrix bigA = Split(A,A.n/threadsNumber,A.m/threadsNumber);
    MatrixOfMatrix bigB = Split(B,B.n/threadsNumber,B.m/threadsNumber);
    return MultipleBig(bigA,bigB,threadsNumber).cast();
}


int main() {
    int Ax,Ay,Bx,By;
    int number_of_threads;
    std::ifstream cin("input.txt");
    cin >> number_of_threads;
    cin >>Ax>>Ay;
    std::vector<std::vector<int>> A(Ax,std::vector<int>(Ay,0));
    for (int i = 0; i < Ax; ++i)
        for (int j = 0; j < Ay; ++j) cin >> A[i][j];

    cin >>Bx>>By;

    std::vector<std::vector<int>> B(Bx,std::vector<int>(By,0));
    for (int i = 0; i < Bx; ++i)
        for (int j = 0; j < By; ++j) cin >> B[i][j];

    if (Ay!=Bx) { std::cout <<"Can't multiple that matrix!"; return 1;}

    auto start = std::chrono::high_resolution_clock::now();
    auto C = SimpleMulitiplication(A, B);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
    for (auto& x:C) {
        for (auto& a:x) {
            std::cout << a << ' ';
        }
        std::cout <<'\n';
    }
    std::cout <<"Spent time: "<<duration.count()<<'\n';

    if (Ax%number_of_threads||
        By%number_of_threads) {
        std::cout <<"Can't multiple by first algorithm\n";
    }
    else {
        std::cout <<"\n\nMultiplication 1\nMatrix:\n";
        auto start = std::chrono::high_resolution_clock::now();
        Matrix x = Calc1(Matrix(A), Matrix(B),number_of_threads);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
        std::cout <<x;
        std::cout <<"Spent time: "<<duration.count()<<'\n';
    }

    if (Ay%number_of_threads||Bx%number_of_threads) {std::cout <<"Can't multiple by second algorithm\n";}
    else {
        std::cout <<"\n\nMultiplication 2\nMatrix:\n";
        auto start = std::chrono::high_resolution_clock::now();
        Matrix x = Calc2(Matrix(A), Matrix(B),number_of_threads);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
        std::cout <<x;
        std::cout <<"Spent time: "<<duration.count()<<'\n';
    }

    if (Ax%number_of_threads||Ay%number_of_threads||Bx%number_of_threads) {std::cout <<"Can't multiple by second algorithm\n";}
    else {
        std::cout <<"\n\nMultiplication 3\nMatrix:\n";
        auto start = std::chrono::high_resolution_clock::now();
        Matrix x = Calc3(Matrix(A), Matrix(B),number_of_threads);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
        std::cout <<x;
        std::cout <<"Spent time: "<<duration.count()<<'\n';
    }

    return 0;
}