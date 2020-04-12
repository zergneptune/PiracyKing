#include "practice.hpp"
#include <stack>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include "utility.hpp"
using std::stack;
//geos
#include <geos.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/operation/buffer/BufferBuilder.h>
#include <geos/index/quadtree/Quadtree.h>
using namespace geos::geom;
using namespace geos::io;
using namespace geos::operation::buffer;
using namespace geos::index::quadtree;
//auto p_global_factory = GeometryFactory::create();

//curses
#include <curses.h>

// test5
int jump(int row, int col)
{
	static int arr[4][5] = {
		0, 1, 2, 3, 4,
		1, 2, 3, 4, 5,
		2, 3, 4, 5, 6,
		3, 4, 5, 6, 7
	};
	static std::list<pair<int, int>> lst_path;
	static int res_cnt = 0;

	if(row > 3 || col > 4)
	{
		return res_cnt;
	}

	lst_path.push_back(std::make_pair(row, col));

	if(arr[row][col] == 7)
	{
		cout << "path:\n";
		for(const auto& ref : lst_path)
		{
			cout << "\t" << ref.first << ", " << ref.second << "\n";
		}
		lst_path.pop_back();
		++ res_cnt;
		return res_cnt;
	}

	jump(row + 1, col);

	jump(row, col + 1);

	lst_path.pop_back();

	return res_cnt;
}

void CreateDeque(BinaryTreeNode* pCurrNode, BinaryTreeNode** pDQueHead)
{
	static BinaryTreeNode* pPreNode = NULL;
	if(pCurrNode == NULL)
	{
		return;
	}
	if(pCurrNode->m_pLeft != NULL)
	{
		CreateDeque(pCurrNode->m_pLeft, pDQueHead);
	}
	if(pPreNode == NULL)
	{
		pPreNode = pCurrNode;
		*pDQueHead = pCurrNode;
		cout << "-" << pCurrNode->m_nValue << endl;
	}
	else
	{
		pCurrNode->m_pLeft = pPreNode;
		pPreNode->m_pRight = pCurrNode;
		pPreNode = pCurrNode;
		cout << "*" << pCurrNode->m_nValue << endl;
	}
	if(pCurrNode->m_pRight != NULL)
	{
		CreateDeque(pCurrNode->m_pRight, pDQueHead);
	}
}

void core(BinaryTreeNode* pNode, int sum)
{
	static int cnt = 0;
	static stack<BinaryTreeNode*> st_node;

	if(pNode == NULL)
	{
		return;
	}
	cnt += pNode->m_nValue;
	st_node.push(pNode);
	cout << "value: " << pNode->m_nValue << ", sum: " << cnt << endl;

	if(pNode->m_pLeft == NULL && pNode->m_pRight == NULL)
	{
		if(cnt == sum)
		{
			cout << "path: ";
			stack<BinaryTreeNode*> st_tmp(st_node);
			while(!st_tmp.empty())
			{
				BinaryTreeNode* pTmp = st_tmp.top();
				cout << pTmp->m_nValue << ',';
				st_tmp.pop();
			}
			cout << endl;
		}
		cnt -= pNode->m_nValue;
		st_node.pop();
		return;
	}

	core(pNode->m_pLeft, sum);

	core(pNode->m_pRight, sum);

	cnt -= pNode->m_nValue;
	st_node.pop();
}

void PrintPath(BinaryTreeNode* pHead, int sum)
{
	core(pHead, sum);
}

bool helper_isPostOrderOfBST(int a[], int low, int high)
{
	int rootNode = a[high];
	int i = low;
	bool lc_tree, rc_tree;

	if(low >= high)
	{
		return true;
	}

	for(; i <= high && a[i] < rootNode; ++i){}
	if(i <= high)
	{
		lc_tree = helper_isPostOrderOfBST(a, low, i - 1);
	}

	int save_i = i;
	for(; i <= high; ++i)
	{
		if(a[i] < rootNode)
		{
			return false;
		}
	}
	rc_tree = helper_isPostOrderOfBST(a, save_i, high - 1);

	return lc_tree && rc_tree;
}

bool isPostOrderOfBST(int a[], int len)
{
	return helper_isPostOrderOfBST(a, 0, len - 1);
}

int* CreateBC(char* pattern, int len)
{
	int* bc = new int[256];

	for(int i = 0; i < 256; ++i)
	{
		bc[i] = -1;
	}

	for(int i = 0; i < len; ++i)
	{
		bc[pattern[i]] = i;
	}

	return bc;
}

int* CreateSuffix(char* pattern, int len)
{
	int* suffix = new int[len];
	suffix[len - 1] = len;

	for(int i = len - 2; i >= 0; --i)
	{
		int j = i;
		for(; pattern[j] == pattern[len - 1 - i + j] && j >= 0; --j);
		suffix[i] = i - j;
	}

	return suffix;
}

int* CreateGS(char* pattern, int len)
{
	int* suffix = CreateSuffix(pattern, len);
	int* gs = new int[len];
	/*
	在计算gs数组时，从移动数最大的情况依次到移动数最少的情况赋值，
	确保在合理的移动范围内，移动最少的距离，避免失配的情况。
	*/
	for(int i = 1; i < len; ++i)
	{
		gs[i] = len;
	}

	for(int i = len - 1; i >= 0; --i) //从右往左扫描，确保模式串移动最少。
	{
		if(suffix[i] == i + 1) //是一个与好后缀匹配的最大前缀
		{
			for(int j = 0; j < len - 1 - i; ++j)
			{
				if(gs[j] == len) //gs[j]初始值为len, 这样确保gs[j]只被修改一次
				{
					gs[j] = len - 1 - i;
				}
			}
		}
	}

	for(int i = 0; i < len - 1; ++i)
	{
		gs[len - 1 - suffix[i]] = len - 1 - i;
	}

	return gs;
}

int bm_search(char* text, int text_len, char* pattern, int pattern_len)
{
	int* bc = CreateBC(pattern, pattern_len);
	int* gs = CreateGS(pattern, pattern_len);

	for(int i = 0; i <= text_len - pattern_len; )
	{
		cout << "i = " << i << endl;
		int j = pattern_len - 1;
		for(; j >= 0 && pattern[j] == text[i+j]; --j);

		if(j < 0)
		{
			return i;
		}

		int bad_char_index = j;
		char bad_char = text[i + j];

		int bc_move = bad_char_index - bc[bad_char];
		if(bc_move < 0)
		{
			bc_move = bad_char_index + 1;
		}

		int gs_move = gs[bad_char_index];

		int move = (bc_move > gs_move ? bc_move : gs_move);

		i += move;
	}

	if(bc != NULL)
	{
		delete bc;
		bc = NULL;
	}

	if(gs != NULL)
	{
		delete bc;
		gs = NULL;
	}
	return -1;
}

int getSum(int n)
{
	int tmpt = n;
	bool bRet = (tmpt > 0) && (n += getSum(n-1));
	return n;
}

int (*getSumFunc[2])(int n);
int fun0(int n){ return 0; }
int fun1(int n){ return n + getSumFunc[!!n](n-1); }
int getSum_2(int n)
{
	getSumFunc[0] = fun0;
	getSumFunc[1] = fun1;

	int nRet = fun1(100);
	return nRet;
}

void T1(int& X, int Y, int i)
{
	bool bRet = (Y & (1<<i)) && (X+=(Y<<i));
}

int getSum_3(int n)
{
	#define T(X, Y, i) (Y & (1<<i)) && (X+=(Y<<i))

	int temp = n;
	T(temp, n, 0);T(temp, n, 1);T(temp, n, 2);T(temp, n, 3);
	T(temp, n, 4);T(temp, n, 5);T(temp, n, 6);T(temp, n, 7);
	T(temp, n, 8);T(temp, n, 9);T(temp, n, 10);T(temp, n, 11);
	T(temp, n, 12);T(temp, n, 13);T(temp, n, 14);T(temp, n, 15);
	T(temp, n, 16);T(temp, n, 17);T(temp, n, 18);T(temp, n, 19);
	T(temp, n, 20);T(temp, n, 21);T(temp, n, 22);T(temp, n, 23);
	T(temp, n, 24);T(temp, n, 25);T(temp, n, 26);T(temp, n, 27);
	T(temp, n, 28);T(temp, n, 29);T(temp, n, 30);T(temp, n, 31);
	return temp>>1;
}

int GetSum::sum = 0;
int GetSum::n = 0;

int lastNumberOfCircle(int n, int m)
{
	if(n == 1)
	{
		return 0;
	}
	return (lastNumberOfCircle(n-1, m) + m) % n;
}

int CountOfBinary1(unsigned long a)
{
	int res = 0;
	while(a)
	{
		a = (a - 1) & a;
		++ res;
	}
	return res;
}

void print(char* fmt, ...)
{

	va_list argptr;
	va_start(argptr, fmt);
	for(char* p = fmt; *p != '\0'; ++p)
	{
		if(*p == '%' && *(p+1) != '%')
		{
			if(*(p+1) == 'd')
			{
				int tmp = va_arg(argptr, int);
				cout << tmp;
				++p;
			}
			else if(*(p+1) == 's')
			{
				const char* tmp = va_arg(argptr, const char*);
				cout << tmp;
				++p;
			}
			else
			{
				cout << *p << *(p+1);
				++p;
			}
		}
		else
		{
			cout << *p;
		}
	}
	va_end(argptr);
}

/*
 * 获取程序的路径名称
 */
char * get_program_path(char *buf,int count)
{
	int i=0;
	int retval = readlink("/proc/self/exe",buf,count-1);
	if((retval < 0 || retval >= count - 1))
	{
	    return NULL;
	}
	//添加末尾结束符号
	buf[retval] = '\0';
	char *end = strrchr(buf,'/');
	if(NULL == end)
	    buf[0] = '\0';
	else
	    *end = '\0';
	return buf;
}

/*
 * 获取这个程序的文件名,其实这有点多余,argv[0] 
 * 就代表这个执行的程序文件名
 */
char * get_program_name(char *buf,int count)
{
	int retval = readlink("/proc/self/exe",buf,count-1);
	if((retval < 0 || retval >= count - 1))
	{
	    return NULL;
	}
	buf[retval] = '\0';
	//获取指向最后一次出现'/'字符的指针
	return strrchr(buf,'/');
}

/*
 * 测试键盘输入
 */
void test_keyboard_input()
{
	fd_set rfds, rs;
	struct timeval tv;

	int i,r,q,j;
	struct termios saveterm, nt;
	char c,buf[32],str[8];

	tcgetattr(0, &saveterm);
	nt = saveterm;

	nt.c_lflag &= ~ECHO;
	nt.c_lflag &= ~ISIG;   
	nt.c_lflag &= ~ICANON; 

	tcsetattr(0, TCSANOW, &nt);

	FD_ZERO(&rs);
	FD_SET(0, &rs);
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
	tv.tv_sec=0;
	tv.tv_usec=0;

	i=0; q=0;
	while(1)
	{
		read(0 , buf+i, 1);
		sprintf(str, "<%X>", *(buf+i));
		i++;
		if(i>31)
		{
			write(1,"Too many data\n",14);
			break;
		}
		write(1, str, 4);
		r = select(0 + 1, &rfds, NULL, NULL, &tv); //0：监听标准输入，若r=1，说明标准输入可读，rfds中标准输入文件描述符会就绪
		if(r<0)
		{
			write(1,"select() error.\n",16);
			break;
		}
		if(r == 1)
			continue;
		write(1, "\t", 1);
		rfds = rs; //恢复rfds，即清除就绪的标准输入文件描述符
		for(j=0; j < i; j++)
		{
			c = buf[j];
			switch(c)
			{
			   	case 27: 
			   		write(1,"ESC ",4);
			        break;
			   	case 9: 
			   		write(1,"TAB ",4);
			        break;
			   	case 32: 
			   		write(1,"SPACE ",6);
			        break;
			    default:
			    	if(c>=32 && c<127)
			    		write(1, buf+j, 1);
			       	else
			       		write(1,"CTRL ",5);
			        break;
			}
		}
		write(1, "\n", 1);
        //确保两次连续的按下ESC键，才退出
		if(buf[0] == 27 && i == 1)
		{
			if(q == 0)
		    	q = 1;
			else
				break;
		}
		else
			q = 0;
		i = 0;
	}

	tcsetattr(0, TCSANOW, &saveterm);
	printf("\n");
}

void printf_format()
{
	printf("\033[30m黑\033[m\n");
    printf("\033[31m红\033[m\n");
    printf("\033[32m绿\033[m\n");
    printf("\033[33m黄\033[m\n");
    printf("\033[34m蓝\033[m\n");
    printf("\033[35m紫\033[m\n");
    printf("\033[36m深绿\033[m\n");
    printf("\033[37m白\033[m\n");

    printf("\033[40m背景黑\033[m\n");
    printf("\033[41m背景红\033[m\n");
    printf("\033[42m背景绿\033[m\n");
    printf("\033[43m背景黄\033[m\n");
    printf("\033[44m背景蓝\033[m\n");
    printf("\033[45m背景紫\033[m\n");
    printf("\033[46m背景深绿\033[m\n");
    printf("\033[47m背景白\033[m\n");


    printf("\033[1;31m红,高亮\033[m\n");
    printf("\033[4;31m红,下划线\033[m\n");
    printf("\033[5;31m红,闪烁\033[m\n");
    printf("\033[7;31m红,反显\033[m\n");
    printf("\033[8;31m红,消隐\033[m\n");
}

void test_geo()
{
    std::cout << "GEOS_VERSION = " << GEOS_VERSION << std::endl;
    /*
    Coordinate coo(13132707.8850685, 4973200.0194835);
    auto point = p_global_factory->createPoint(coo);
    if (point->isEmpty())
    {
        std::cout << "empty point" << std::endl;
        return;
    }

    if (point->isSimple())
    {
        std::cout << "is simple point" << std::endl;
    }

    std::cout << "the num of point is " << point->getNumPoints() << std::endl;
    std::cout << "the point is " << point->getCoordinateDimension() << " dimension coordinate" << std::endl;
    std::cout << "x: " << point->getX() << " y: " << point->getY() << std::endl;
    std::cout << "string : " << point->toString() << std::endl;
    std::cout << "SRID : " << point->getSRID() << std::endl;
    */

    //WKT WKB转换
    WKBWriter wkb_writer;
    std::ostringstream ostr;
    //wkb_writer.writeHEX(*point, ostr);
    //std::string wkt_str = wkt_writer.write(point);
    //std::cout << "WKB hex : " << ostr.str() << std::endl;
    //std::cout << "WKT string : " << wkt_str << std::endl;

    WKBReader wkb_reader;
    std::istringstream istr(ostr.str());
    istr.str("0101000000377B527C740C6941BB373F01A4F85241");
    auto ret_geo = wkb_reader.readHEX(istr);
    std::cout << "return geo is " << ret_geo->toString() << std::endl;

    std::vector<std::string> vec_wkb_hex{
        "0101000000377B527C740C6941BB373F01A4F85241",
        "010100000010A8CC241F0B69410C1F34E53F015341",
        "01010000002BCB0E9F1D0B6941E5039AC61C005341",
        "0101000000D623E7F6F40A6941501DC8FA54015341",
        "010100000084F6F671830A6941783C0869AC015341",
        "010100000000000020FF0B6941000000C0A0045341",
        "0101000000666666566E1669419A999959B69F5341",
        "010100000000000000310B6941000000605A055341",
        "01010000002FE97246010C6941D795BE2B26FA5241",
        "0101000000000000C0DD16694100000060B8A05341"
    };

    int i = 0;
    std::vector<const Coordinate*> vec_coo;
    std::vector<Geometry*> vec_geo;
    for (const auto& wkb_hex : vec_wkb_hex)
    {
        std::istringstream istr;
        istr.str(wkb_hex);
        auto res_geo = wkb_reader.readHEX(istr);
        std::cout << ++i << " geo is " << res_geo->toString() << std::endl;
        std::cout << "GeometryTypeId = " << res_geo->getGeometryTypeId() << " GeometryType = " << res_geo->getGeometryType() << std::endl;
        const auto coo = res_geo->getCoordinate();
        std::cout << "x: " << coo->x << " y: " << coo->y << std::endl;
        vec_geo.emplace_back(res_geo);
        vec_coo.emplace_back(coo);
    }

    //计算距离
    std::cout << "distance = " << vec_coo[0]->distance(*vec_coo[1]) << std::endl;
    std::cout << "distance = " << vec_geo[0]->distance(vec_geo[1]) << std::endl;


    //buffer
    BufferParameters buffer_param(8);//quadrant segments is 8(default)
    BufferBuilder buffer_builder(buffer_param);
    std::unique_ptr<const Geometry> geo_buffer(buffer_builder.buffer(vec_geo[0], 9300));
    if (geo_buffer->intersects(vec_geo[1]))
    {
        std::cout << "geo_buffer intersects the geo!" << std::endl;
    }

    //BufferBuilder不能重复用 BufferParameters可以
    BufferBuilder buffer_builder_2(buffer_param);
    std::unique_ptr<const Geometry> geo_buffer_2(buffer_builder_2.buffer(vec_geo[0], 9300));
    if (geo_buffer_2->intersects(vec_geo[1]))
    {
        std::cout << "geo_buffer intersects the geo!" << std::endl;
    }

    for (const auto& p_geo : vec_geo)
    {
        std::cout << "geo_buffer distance to geo " << p_geo << " = " << p_geo->distance(geo_buffer.get()) << std::endl;
    }

    //构建四插树空间索引
    Quadtree quad_tree;
    for (auto p_geo : vec_geo)
    {
        auto p_envel = p_geo->getEnvelopeInternal();
        quad_tree.insert(p_envel, static_cast<void*>(p_geo));
    }
    std::cout << "the depth of quadtree is " << quad_tree.depth() << std::endl;
    std::cout << "the size of quadtree is " << quad_tree.size() << std::endl;

    Envelope search_envel(*geo_buffer->getEnvelopeInternal());
    std::vector<void*> vec_ret;
    quad_tree.query(&search_envel, vec_ret);
    std::cout << "the size of vec_ret is " << vec_ret.size() << std::endl;
    for (auto p_void : vec_ret)
    {
        const auto p_geo = static_cast<Geometry*>(p_void);
        if (p_geo)
        {
            //std::cout << "return geo = " << p_geo << " distance = " << p_geo->distance(geo_buffer.get()) << std::endl;
            std::cout << "return geo = " << p_geo << " distance = " << p_geo->distance(vec_geo[0]) << std::endl;
        }
    }

    CalcTimeFuncInvoke(
        for (int i = 0; i < 1000000; ++i)
        {
            WKBReader wkb_reader;
            std::istringstream istr(ostr.str());
            istr.str("0101000000377B527C740C6941BB373F01A4F85241");
            auto ret_geo = wkb_reader.readHEX(istr);
        },
        "wkb_reader 100w"
    );
    
    /*
    if (!point)
    {
        delete point;
        point = nullptr;
    }
    */

    for (auto p_geo : vec_geo)
    {
       if (!p_geo)
       {
           delete p_geo;
           p_geo = nullptr;
       }
    }

    for (auto p_coo : vec_coo)
    {
       if (!p_coo)
       {
           delete p_coo;
           p_coo = nullptr;
       }
    }

    if (!ret_geo)
    {
        delete ret_geo;
        ret_geo = nullptr;
    }

}

void initial()
{
    initscr();                    //开启curses模式
 
    cbreak();                     //开启cbreak模式，除 DELETE 或 CTRL 等仍被视为特殊控制字元外一切输入的字元将立刻被一一读取
 
    nonl();                       //用来决定当输入资料时，按下 RETURN 键是否被对应为 NEWLINE 字元
 
    noecho();                     //echo() and noecho(): 此函式用来控制从键盘输入字元时是否将字元显示在终端机上
 
    intrflush(stdscr,false);
 
    keypad(stdscr,true);          //当开启 keypad 後, 可以使用键盘上的一些特殊字元, 如上下左右>等方向键
 
    refresh();                    //将做清除萤幕的工作
}

void test_curses()
{
  	initscr();
 
    box(stdscr, ACS_VLINE, ACS_HLINE); /*draw a box*/

    move(LINES/2, COLS/2); /*move the cursor to the center*/
 
    waddstr(stdscr, "Hello, world!");
 
    refresh();
 
    getch();
 
    endwin();  
}

void test_curses2()
{
    initscr(); /*初始化屏幕*/
    box(stdscr, '*', '*'); /*draw a box*/
    move(LINES/2, COLS/2);
 
    if(start_color() == OK) /*开启颜色*/
    {
     
        init_pair(1, COLOR_RED, COLOR_GREEN); /*建立一个颜色对*/
     
		attron(COLOR_PAIR(1)); /*开启字符输出颜色*/
	 
		waddstr(stdscr, "Yet another Hello, world!");
			 
		attroff(COLOR_PAIR(1)); /*关闭颜色显示*/
			 
		refresh();
			 
		getch();
	}
	else
	{
			 
		waddstr(stdscr, "Can not init color");
				 
		refresh();
				 
		getch();
		 
	}
		 
	endwin(); /*关闭curses状态*/
}
