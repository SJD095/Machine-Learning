//13331233 孙中阳 计应2
//szy@sunzhongyang.com 13717558798

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <unordered_map>
#include <cstdlib>
#include <ctime>

using namespace std;

const int EXAMPLE_AMOUNT = 2177020; //样例数量
const int OUTPUT_AMOUNT = 220245; //测试集数量
const int ATTRIBUTION_AMOUNT = 11392;  //属性数量
const int THREAD_AMOUNT = 8;    //同时运行的线程的数量
int TREE_PER_THREAD = 6;    //每个线程所需要完成的树的数量
const int EXAMPLE_SELECT_AMOUNT = 20000;    //一棵树需要的样例个数
int ATTRIBUTION_SELECT_AMOUNT =100;  //一棵树需要的属性的个数
const string TRAINING_FILE_NAME = "train.txt";  //输入数据的文件名
const string TEST_FILE_NAME = "test.txt"; //测试集文件名
unordered_map<int, unordered_map<int, bool> > TRAINING_DATA;  //保存训练数据的hash map
unordered_map<int, unordered_map<int, bool> > TEST_DATA; //保存测试数据的hash map
unordered_map<int, bool> TRAINING_REAULT; //保存训练数据最终结果（1或0）的hash map
vector<bool> OUTPUT; //最后直接输出了，没有用到这个vector
vector<int> ZERO_ATTR(ATTRIBUTION_AMOUNT, 0);  //经观察，有的数据在所有训练数据上都是0，因此在选取构造决策树的属性时直接忽略掉这些属性
int tree_count = 0;

class node
{
public:
    node* left_child = NULL;  //左子节点，如果代表属性为1的边，也就是说在节点对应属性为1的情况下会通向左子节点
    node* right_child = NULL; //右子节点
    int attribution_number = 0; //节点对应的属性编号，从0到11391
    bool leaf_node = false; //标记是否为叶子节点，默认为否
    int leaf_result = -1; //如果是叶子节点，决定叶子节点的值（0或1），默认为-1
    vector<int> example_id, attribution_id; //构造树时作为构造参照的样例ID和属性ID，根据上述数据计算GINI指数，然后进一步划分

    int caculate_id();  //根据example_id和attribution_id计算GINI最小的属性的ID，并返回该属性的ID
    bool is_leaf_node();  //判断当前节点是否为叶子节点
    int bianli(int example_judge); //遍历训练数据，获得准确度的参考
    int bianli2(int training_id); //遍历测试数据，输出结果到output.txt
};

vector<node*> trees;  //保存生成的树

int node::bianli(int example_judge)
{
    //递归直到某个叶子节点，返回叶子节点对应的leaf_result
    if(leaf_node == true) return leaf_result;
    else
    {
      //这里用find(属性ID)的原因是training.txt里面只给了值为1的属性ID，所以能找到的属性ID对应的属性值一定为1，找不到的就一定为0
        if(TRAINING_DATA[example_judge].find(attribution_number) != TRAINING_DATA[example_judge].end()) return left_child->bianli(example_judge);
        else  return right_child->bianli(example_judge);
    }
}

int node::bianli2(int example_judge)
{
    if(leaf_node == true) return leaf_result;
    else
    {
        if(TEST_DATA[example_judge].find(attribution_number) != TEST_DATA[example_judge].end()) return left_child->bianli2(example_judge);
        else  return right_child->bianli2(example_judge);
    }
}

void read_data(string file_name) //此处可能有巨大的传递开销
{
  //读取数据，可根据file_name决定读取训练数据或者测试数据
    int count = 0;  //标记已经读取的数据的数目
    unordered_map<int, unordered_map<int, bool> > result;
    ifstream input(file_name);
    string line, tmp_tuple;
    while(getline(input, line))
    {
        cout << count << endl;
        istringstream string_line(line);  //采用istringstream规避空格

        if(file_name == TRAINING_FILE_NAME)
       {
            string_line >> tmp_tuple; //训练数据是带结果的，优先保存结果
            if(tmp_tuple == "1") TRAINING_REAULT[count] = true; //本程序中大量采用bool值保存数据，这样可节省一定的内存空间
            else TRAINING_REAULT[count] = false;

            while(string_line >> tmp_tuple)
            {   //保存属性值为1的属性的ID
                int pos = tmp_tuple.find(':');
                string s1;
                s1 = tmp_tuple.substr(0, pos);
                TRAINING_DATA[count][atoi(s1.c_str())] = true;
                ZERO_ATTR[atoi(s1.c_str())] = 1; //ZERO_ATTR中属性为1的在选取属性构造决策树时才会被选中
            }
       }

        if(file_name == TEST_FILE_NAME)
        {
            string_line >> tmp_tuple;
            while(string_line >> tmp_tuple)
            {
                int pos = tmp_tuple.find(':');
                string s1;
                s1 = tmp_tuple.substr(0, pos);
                TEST_DATA[count][atoi(s1.c_str())] = true;
            }
       }

        count++;
    }
}

vector<int> getRandom2(int all, int select)
{
  //这里稍微复杂一点，目的是获取不重复的随机数
  vector<int> input;
  for (int i = 0; i < all; i++)
  {
    input.push_back(i);
  }
  vector<int> output;

  int end = all;
  for (int i = 0; i < select; i++)
  {
    auto iter = input.begin();
    int num = random()%end;
    iter = iter+num;
    while(ZERO_ATTR[(*iter)] == 0)
    {
      //这里的目的是不选取ZERO_ATTR中标记的属性ID
        num = random()%end;
        iter = input.begin()+num;
    }
    output.push_back(*iter);
    input.erase(iter);
    end--;
  }

  return output;
}

vector<int> getRandom(int all, int select)
{
  //这里选取的是样例N，由于是放回后抽取，所以可以选择重复的
  vector<int> out;
  for(int i = 0; i < select; i++)
  {
    out.push_back(random()%all);
  }
  return out;
}

bool node::is_leaf_node()
{   //判断当前节点是否有作为叶子节点的条件
  if(attribution_id.size() == 0)
  {
    //这里判断是不是分配到的属性已经没有了，如果属性已用尽，那么一定是叶子节点，即使剩余的样例结果同时有1和0，这时可根据1和0哪个更多来决定leaf_result
    leaf_node = true;
    int one_number = 0;
    int zero_number = 0;
    for(auto c: example_id)
    {
      if(TRAINING_REAULT[c] == 1) one_number++;
      else zero_number++;
    }

    if(one_number >= zero_number) leaf_result = 1;
    else leaf_result = 0;
    return true;
  }

  else
  {
    //另一种情况则是分配到的训练数据的结果全部为0或者全部为1，这时可判定为叶子节点
    int last = TRAINING_REAULT[example_id[0]];
    for(auto c: example_id)
    {
      if(last != TRAINING_REAULT[c]) return false;
      last = TRAINING_REAULT[c];
    }

    leaf_node = true;
    leaf_result = TRAINING_REAULT[example_id[0]];
    return true;
  }
}

int node::caculate_id()
{
  double last = 1;
  int result = -1;
  double one_one = 0; //属性1，结果1
  double one_total = 0; //属性1，结果0或1
  double zero_one = 0;  //属性0，结果1
  double zero_total = 0;  //属性0，结果0或1
  //对每个属性ID，判断根据这个ID进行划分后已有训练数据的划分情况
  for(auto c: attribution_id)
  {
    for(auto d: example_id)
    {
      if(TRAINING_DATA[d].find(c) != TRAINING_DATA[d].end())
      {
        if(TRAINING_REAULT[d] == true) one_one++;
        one_total++;
      }

      else
      {
        if(TRAINING_REAULT[d] == true) zero_one++;
        zero_total++;
      }
    }

    double gini1, gini2;

    //首先需要注意的是，one_total和zero_total是否为0的判断，否则会有一定概率出现除0的错误
    if(one_total != 0)
    {
        gini1 = 1 - (one_one/one_total) * (one_one/one_total) - ((one_total - one_one)/one_total) * ((one_total - one_one)/one_total);
    }
    else gini1 = 0;

    if(zero_total != 0)
    {
        gini2 = 1 - (zero_one/zero_total) * (zero_one/zero_total) - ((zero_total - zero_one)/zero_total) * ((zero_total - zero_one)/zero_total);
    }

    else gini2 = 0;

    //计算GINI
    double gini = gini1 * (one_total/(zero_total + one_total)) + gini2 * (zero_total/(zero_total + one_total));

    if(gini < last)
    {
      last = gini;
      result = c;
    }

    one_one = 0;
    one_total = 0;
    zero_one = 0;
    zero_total = 0;
  }
  return result;
}

node* create_tree(vector<int> example_id, vector<int> attribution_id)
{
  //构造树是递归的，先创建属于自己的节点，判断自己是不是叶子节点，如果不是，那么计算最优的用于划分的属性，然后根据属性分派自己的到的example_id和attribution_id到左子节点和右子节点。如果是叶子节点则返回
  node* root = new node();
  root->example_id = example_id;
  root->attribution_id = attribution_id;
  if(root->is_leaf_node()) return root;
  root->attribution_number = root->caculate_id();

  vector<int> left_example, right_example;

  //划分训练数据
  for(auto c: example_id)
  {
    if(TRAINING_DATA[c].find(root->attribution_number) != TRAINING_DATA[c].end()) left_example.push_back(c);
    else right_example.push_back(c);
  }

  //这里需要加一个判断，就是划分到左子节点或者右子节点的训练数据的个数是否为0，如果为0则也是叶子节点
  if(left_example.size() == 0 || right_example.size() == 0)
  {
    root->leaf_node = true;
    int one_number = 0;
    int zero_number = 0;
    for(auto c: example_id)
    {
      if(TRAINING_REAULT[c] == true) one_number++;
      else zero_number++;
    }

    if(one_number >= zero_number) root->leaf_result = 1;
    else root->leaf_result = 0;
    return root;
  }
  //重新计算属性
  auto it = attribution_id.begin();
  while(it != attribution_id.end())
  {
    if((*it) == root->attribution_number)attribution_id.erase(it);
    else it++;
  }

  vector<int> new_attr = attribution_id;
  root->left_child = create_tree(left_example, new_attr);
  root->right_child = create_tree(right_example, new_attr);

  return root;
}

void build_tree()
{
    //构造树，这里不是递归的，可直接获取一个根节点，首先会计算出随机的example_id和attribution_id给根节点
    vector<int> example_id = getRandom(EXAMPLE_AMOUNT, EXAMPLE_SELECT_AMOUNT);
    vector<int> attribution_id = getRandom2(ATTRIBUTION_AMOUNT, ATTRIBUTION_SELECT_AMOUNT);

    node* tree = create_tree(example_id, attribution_id);

    trees.push_back(tree);
    tree_count++;
    cout << "tree complete " << tree_count << '/' << TREE_PER_THREAD * THREAD_AMOUNT << endl;
}

void thread_function()
{
  //每个线程构造一些树，同时有一个字节的seed
    srandom(time(0));

    for(int i = 0; i < TREE_PER_THREAD; i++)
    {
        build_tree();
    }

}

int main()
{
    vector<thread> threads; //若干个线程

    read_data(TRAINING_FILE_NAME);  //读训练数据
    read_data(TEST_FILE_NAME);  //读测试数据
    cout << "success" << endl;

    while(true)
    {
      //不断训练树并输出
	cin >> TREE_PER_THREAD >> ATTRIBUTION_SELECT_AMOUNT;  //输入每个线程计算多少树，每棵树有多少个属性
	for(int i = 0; i < THREAD_AMOUNT; i++)
    	{
        	threads.push_back(thread(thread_function));
    	}

        for(auto& t: threads)
        {
            t.join();
        }

        int t = 0, f = 0;
        for(int i = 1100000; i < 1110000; i++)
        {
          //这里主要是根据训练集进行一个判断，看看采用的树的数量和属性数量是否靠谱
            int one_count = 0, zero_count = 0, out = 0;
            for(auto tree: trees)
            {
                int tmp_thread = tree->bianli(i);
                if(tmp_thread == 1) one_count++;
                else zero_count++;
            }
            cout << "one: " << one_count << endl;
            if(one_count >= zero_count) out = 1;

            if((out == 1 && TRAINING_REAULT[i] == true) || (out == 0 && TRAINING_REAULT[i] == false)) t++;
            else f++;
        }

        cout << "t " << t << "f " << f << endl;

        //后面才是用于输出结果的
        ofstream output("output.txt");
        output << "id,label" << endl;

        for(int i = 0; i < OUTPUT_AMOUNT; i++)
        {
            int one_count = 0, zero_count = 0, out = 0;
            for(auto tree: trees)
            {
                int tmp_thread = tree->bianli2(i);
                if(tmp_thread == 1) one_count++;
                else zero_count++;
            }
            if(one_count >= zero_count) out = 1;
            output << i << ',' << out << endl;
        }
        tree_count = 0;
	     trees.clear();
	     threads.clear();
    }
}

