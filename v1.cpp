#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <map>
#include <algorithm>
#include <iterator>
#include <vector>
#include <time.h>
using namespace std;
#define maxNum 5000
#define rankMode 0
#define clusterNum 3
#define loopNum 1
#define emNum 5

int adjXY[maxNum][maxNum];    // Wxy mentioned in the paper
int adjYY[maxNum][maxNum];    // Wyy mentioned in the paper
int paperNum[maxNum]; // the paper number per conference

map<string, int> conf;        // a hash table for conferences
map<string, int> author;      // a hash table for authors
int authorNum = 0;            // the number of authors
int confNum = 0;              // the number of conferences
double rankX[clusterNum][maxNum];
double rankY[clusterNum][maxNum];
int confCluster[maxNum];

double PZ[clusterNum];
double conditionalPZ[maxNum][maxNum][clusterNum];
double theta[maxNum][clusterNum];
/*
* split a line by a ','
*/
void split(std::string& s,std::vector< std::string >* ret) 
{  
    size_t last = 0;  
    string delim = ",";
    size_t index=s.find_first_of(delim,last);
    ret->clear(); 
    while (index!=std::string::npos)  
    {  
        ret->push_back(s.substr(last,index-last));  
        last=index+1;  
        index=s.find_first_of(delim,last);  
    }  
    if (index-last>0)  
    {  
        ret->push_back(s.substr(last,index-last));  
    }  
}

/*
* Initialization:
* assign each target object with a cluster
* label from 1 to K randomly, no empty cluster
*/
void Init()
{
    bool notepad[clusterNum];
    bool flag = true;
    srand((int)time(0));
    while(true)
    {
        flag = true;
        memset(notepad, 0, sizeof(notepad));
        for(int i = 0; i < confNum; ++i)
        {
            confCluster[i] = rand() % clusterNum;
            notepad[confCluster[i]] = true;
        }
        for(int i = 0; i < clusterNum; ++i)
        {
            if(!notepad[i])
            {
                flag = false;
                break;
            }
        }
        if(flag)
            break;
    }
}

/*
* simple rank function
*/
void simpleRank()
{
    double sum;
    double tmp;
    for(int k = 0; k < clusterNum; ++k)
    {
        sum = 0;
        for(int i = 0; i < confNum; ++i)
        {
            if(confCluster[i] != k)
                continue;
            for(int j = 0; j < authorNum; ++j)
                sum += adjXY[i][j];
        }

        /* conditional rank on the author */
        for(int i = 0; i < authorNum; ++i)
        {
            tmp = 0;
            for(int j = 0; j < confNum; ++j)
            {
                if(confCluster[j] != k)
                    continue;
                tmp += adjXY[j][i];
            }
            rankY[k][i] = tmp / sum;
        }

        sum = 0;
        for(int i = 0; i < confNum; ++i)
        {
            for(int j = 0; j < authorNum; ++j)
                sum += adjXY[i][j]*rankY[k][j];
        }
        /* conditional rank on the conference */
        for(int i = 0; i < confNum; ++i)
        {
            tmp = 0;
            for(int j = 0; j < authorNum; ++j)
                tmp += adjXY[i][j]*rankY[k][j];
            
            rankX[k][i] = tmp / sum;
        }
    }
    return;
}
void authorityRank()
{
    return;
}
void Rank(int mode)
{
    if(mode == 0)
        simpleRank();
    else
        authorityRank();
}
/*
* EM algorithm
* The array PZ is p(z = k), the array conditionalPZ
* is p(z = k | y,x,theta).
*/
void EM()
{
    /*calculate p0(z = k)*/
    double sum = 0;
    double tmp = 0;
    
    for(int i = 0; i < confNum; ++i)
        sum += paperNum[i];
    for(int k = 0; k < clusterNum; ++k)
    {
        tmp = 0;
        for(int i = 0; i < confNum; ++i)
        {
            if(confCluster[i] == k)
                tmp += paperNum[i];
        }
        PZ[k] = tmp  / sum;
    }

    int sumWxy = 0;                                 //sum of the matirx Wxy, used in M-step
    for(int i = 0; i < confNum; ++i)
    {
        for(int j = 0; j < authorNum; ++j)
            sumWxy += adjXY[i][j];
    }

    for(int t = 0; t < emNum; ++t)
    {
        /* E-step */
        for(int i = 0; i < confNum; ++i)
        {
            for(int j = 0; j < authorNum; ++j)
            {
                sum = 0;
                for(int k = 0; k < clusterNum; ++k)
                {
                    conditionalPZ[i][j][k] = rankX[k][i]*rankY[k][j]*PZ[k];
                    sum += conditionalPZ[i][j][k];
                }
                for(int k = 0; k < clusterNum; ++k)
                {
                    conditionalPZ[i][j][k] /= sum;
                }
            }
        }

        /* M-step */
        for(int k = 0; k < clusterNum; ++k)
        {
            tmp = 0;
            for(int i = 0; i < confNum; ++i)
            {
                for(int j = 0; j < authorNum; ++j)
                    tmp += adjXY[i][j]*conditionalPZ[i][j][k];
            }
            PZ[k] = tmp / sumWxy;
        }
    }

    /*calculate the mathix *theta* */
    for(int i = 0; i < confNum; ++i)
    {
        for(int k = 0; k < clusterNum; ++k)
        {
            sum = 0;
            for(int l = 0; l < clusterNum; ++l)
            {
                sum += rankX[l][i]*PZ[l];
            }
            theta[i][k] = rankX[k][i]*PZ[k]/sum;
        }
    }
    return;
}
void Cluster()
{
    return;
}
int main()
{
    ifstream infile;
    infile.open("test.txt");                // open the file with dblp data
    if(!infile)
    {
        cout << "error" << endl;
        return 0;
    }
    conf.clear();
    author.clear();

    /*create Wxy and Wyy*/
    string line;                                    // a line in the file
    vector<string> content;                         // the result after *split* function
    
    while(getline(infile, line))
    {
        if(line.size() == 0)
            continue;
        
        split(line, &content);
        if(conf.find(content[0]) == conf.end())     // a new conference
            conf[content[0]] = confNum++;
        
        paperNum[conf[content[0]]]++;
        for(int i = 1; i < content.size(); ++i)
        {
            if(author.find(content[i]) == author.end()) // a new author
                author[content[i]] = authorNum++;
            adjXY[conf[content[0]]][author[content[i]]]++;
            for(int j = 1; j < i; ++j)
            {
                adjYY[author[content[i]]][author[content[j]]]++;
                adjYY[author[content[j]]][author[content[i]]]++;
            }
        }
    }

    /*initiation*/
    Init();
    for(int i = 0; i < loopNum; ++i)
    {
        cout << "----------Loop:" << i << "----------" << endl;
        cout << "Start Ranking..." << endl;
        Rank(rankMode);
        cout << "Start EM..." << endl;
        EM();
        cout << "Clustering..." << endl;
        Cluster();
    }
    infile.close();
    return 0;
}