#include <iostream>
#include <string>
#include <algorithm>
#include <mpi.h>
using namespace std;

int max(int a,int b,int c){
  return max(a,max(b,c));
}
int isParallel(int *a,int *b,int len){
  int offset=a[0]-b[0];
  for(int i=1;i<=len;i++){
    if(a[i]-b[i]!=offset)
    return 0;
  }
  return 1;
}
int *calc_row(int *score,int len1,string a,string b,int ii,int match,int mismatch,int gap,int *dir){
  int *ans=(int*)malloc((len1+1)*sizeof(int));
  for(int i=0;i<=len1;i++){
    if(i==0){
      ans[0]=score[0]+gap;
    }else{
      int val=0;
      if(a.at(i-1)==b.at(ii-1))
      val=match;
      else
      val=mismatch;
      ans[i]=max(ans[i-1]+gap,score[i]+gap,score[i-1]+val);
      if(ans[i]==(ans[i-1]+gap)){
        dir[i]=0;
      }else if(ans[i]==(score[i]+gap)){
        dir[i]=2;
      }else{
        dir[i]=1;
      }
    }
  }
  return ans;
}
int findConv(int conv[],int l,int r){
  for(int i=l;i<r;i++){
    if(conv[i]==0)
    return 0;
  }
  return 1;
}

int main(int argc, char** argv){
  if (argc != 6) {
    fprintf(stderr, "Usage: String1 String2 Match Mismatch Gap\n");
    exit(1);
  }

  MPI_Init(NULL, NULL);
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  string a=argv[1];
  string b=argv[2];
  int len1=a.size();
  int len2=b.size();
  int match=atoi(argv[3]);
  int mismatch=atoi(argv[4]);
  int gap=atoi(argv[5]);
  int nop[world_size];
  for(int i=0;i<world_size;i++){
    nop[i]=(len2)/world_size;
  }
  for(int i=0;i<(len2)%world_size;i++){
    nop[i]++;
  }
  int lp[world_size];
  lp[0]=1;
  for(int i=1;i<world_size;i++){
    lp[i]=lp[i-1]+nop[i-1];
  }

  int *score[len2+1];
  int conv[len2+1];
  int dirx[]={-1,-1,0};
  int diry[]={0,-1,-1};
  int *dir[len2+1];
  for(int i=0;i<=len2;i++){
    score[i]=(int*)calloc((len1+1),sizeof(int));
    dir[i]=(int*)calloc((len1+1),sizeof(int));
    conv[i]=0;
  }
  score[0][0]=0;
for(int i=1;i<=len1;i++){
  score[0][i]=score[0][i-1]+gap;
  dir[0][i]=0;
}
conv[0]=1;
for(int i=1;i<=len2;i++){
  score[i][0]=score[i-1][0]+gap;
  dir[i][0]=2;
}
int sending=1,receiving=1;
if(world_rank==0)
receiving=0;
if(world_rank==world_size-1)
sending=0;

int allconv=findConv(conv,lp[world_rank],lp[world_rank]+nop[world_rank]);
while(!allconv){
  for(int i=lp[world_rank];i<lp[world_rank]+nop[world_rank];i++){
    if(allconv)
    break;
    int *temp=NULL;
    if(i==lp[world_rank]){
      int *recv=(int*)malloc((len1+1)*sizeof(int));
      if(sending){
        MPI_Send(score[lp[world_rank]+nop[world_rank]-1], len1+1, MPI_INT, world_rank+1, 0, MPI_COMM_WORLD);
        MPI_Send(&sending, 1, MPI_INT, world_rank+1, 1, MPI_COMM_WORLD);
      }
      if(receiving){
        //cout<<world_rank<<" "<<i<<endl;
        MPI_Recv(recv,len1+1,MPI_INT,world_rank-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(&receiving,1,MPI_INT,world_rank-1,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        temp=calc_row(recv,len1,a,b,i,match,mismatch,gap,dir[i]);
      }else{
        if(i==1)
        temp=calc_row(score[i-1],len1,a,b,i,match,mismatch,gap,dir[i]);
        else
        temp=score[i];
      }
    }
    else
    temp=calc_row(score[i-1],len1,a,b,i,match,mismatch,gap,dir[i]);
    // if(i==1){
    //   for(int x=0;x<=len1;x++){
    //     cout<<score[i][x]<<" ";
    //   }
    //   cout<<endl;
    // }
    if(isParallel(temp,score[i],len1)){
      for(int j=i;j<lp[world_rank]+nop[world_rank];j++){
        conv[j]=1;
      }
      if(world_rank!=world_size-1){
        MPI_Send(score[lp[world_rank]+nop[world_rank]-1], len1+1, MPI_INT, world_rank+1, 0, MPI_COMM_WORLD);
        sending=0;
        MPI_Send(&sending, 1, MPI_INT, world_rank+1, 1, MPI_COMM_WORLD);
      }
      break;
    }
    free(score[i]);
    score[i]=temp;
  }
  allconv=findConv(conv,lp[world_rank],lp[world_rank]+nop[world_rank]);
}

  MPI_Barrier(MPI_COMM_WORLD);
  if(world_rank==0){
    for(int i=1,j=lp[i];j<=len2;j++){
      if(i<world_size-1&&j>=lp[i+1]){
        i++;
      }
      MPI_Recv(score[j],len1+1,MPI_INT,i,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(dir[j],len1+1,MPI_INT,i,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
  }else{
    for(int i=lp[world_rank];i<lp[world_rank]+nop[world_rank];i++){
      MPI_Send(score[i], len1+1, MPI_INT, 0, 3, MPI_COMM_WORLD);
      MPI_Send(dir[i], len1+1, MPI_INT, 0, 2, MPI_COMM_WORLD);
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  if(world_rank==0){
    //cout<<len1<<" "<<len2;
    // for(int i=0;i<=len2;i++){
    //   for(int j=0;j<=len1;j++){
    //     cout<<score[i][j]<<" ";
    //   }
    //   cout<<endl;
    // }
    // cout<<endl;
    // for(int i=0;i<=len2;i++){
    //   for(int j=0;j<=len1;j++){
    //     cout<<dir[i][j]<<" ";
    //   }
    //   cout<<endl;
    // }
    string res1="",res2="";

    while(!(len1==0&&len2==0)){
      //cout<<len2<<" "<<len1<<" "<<dir[len2][len1]<<endl;
      switch(dir[len2][len1]){
        case 2:
        res1+='-';
        res2+=b.at(len2-1);
        break;
        case 1:
        res1+=a.at(len1-1);
        res2+=b.at(len2-1);
        break;
        case 0:
        res1+=a.at(len1-1);
        res2+='-';
      }
      int x=len2+diry[dir[len2][len1]];
      int y=len1+dirx[dir[len2][len1]];
      len1=y;
      len2=x;
    }
    reverse(res1.begin(),res1.end());
    reverse(res2.begin(),res2.end());
    cout<<"Final Allignment: "<<endl;
    cout<<res1<<endl;
    cout<<res2<<endl;
  }
  MPI_Finalize();
}
