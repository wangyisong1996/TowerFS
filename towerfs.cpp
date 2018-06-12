#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <assert.h>

// TODO: Add destructors

// ===========================
// Game Config

struct Location {
	int id_row, id_col;
	
	void init() {
		id_row = 1;
		id_col = 1;
	}
	
	void next_line() {
		id_row++;
		id_col = 1;
	}
	
	void next_char() {
		id_col++;
	}
};

struct Scanner {
	FILE *f;
	Location loc;
	
	char last_ch;
	
	Scanner(FILE *f) {
		this->f = f;
		this->loc.init();
		this->last_ch = 0;
	}
	
	Location get_location() {
		return loc;
	}
	
	char do_get_char() {
		char ch = fgetc(f);
		if (ch == '\r') {
			ch = '\n';
		}
		if (ch == '\n') {
			loc.next_line();
		} else {
			loc.next_char();
		}
		return ch;
	}
	
	char get_char() {
		if (last_ch == EOF) {
			return EOF;
		}
		// Treat \r as \n, and \n \n ... \n as \n
		char ch = do_get_char();
		while (last_ch == '\n' && ch == '\n') {
			ch = do_get_char();
		}
		last_ch = ch;
		return ch;
	}
	
	// TODO: Error handling
	int read_int(int l = -2147483648, int r = 2147483647) {
		char ch = get_char();
		while (ch <= 32) {
			ch = get_char();
		}
		int ret = 0;
		bool f = ch == '-';
		if (f) {
			ch = get_char();
		}
		while (ch > 32) {
			ret = ret * 10 + ch - 48;
			ch = get_char();
		}
		return f ? -ret : ret;
	}
	
	// TODO: Error handling
	std::string read_line() {
		char ch = get_char();
		while (ch == '\n') {
			ch = get_char();
		}
		std::string ret;
		while (ch != '\n' && ch != EOF) {
			ret += ch;
			ch = get_char();
		}
		return ret;
	}
	
	// TODO: Error handling
	std::string read_string() {
		char ch = get_char();
		while (ch == '\n' || ch == ' ') {
			ch = get_char();
		}
		std::string ret;
		while (ch != '\n' && ch != ' ' && ch != EOF) {
			ret += ch;
			ch = get_char();
		}
		return ret;
	}
};

struct MonsterConfig {
	std::string name;
	std::string short_name;
	int HP, ATK, DEF, GOLD;
	
	MonsterConfig(Scanner *s) {
		name = s->read_line();
		short_name = s->read_line();
		HP = s->read_int();
		ATK = s->read_int();
		DEF = s->read_int();
		GOLD = s->read_int();
	}
};

struct NodeEventConfig {
	std::string type;
	
	int n_monsters;
	std::vector<std::string> monsters;
	
	int n_obj;
	std::vector<std::string> obj;
	
	int n_doors;
	std::vector<std::string> doors;
	
	NodeEventConfig() {
		type = "none";
	}
	
	NodeEventConfig(Scanner *s) {
		type = s->read_line();
		if (type == "none") {
			// None
		} else if (type == "door") {
			n_doors = s->read_int();
			doors.clear();
			for (int i = 0; i < n_doors; i++) {
				doors.push_back(s->read_line());
			}
		} else if (type == "monster") {
			n_monsters = s->read_int();
			monsters.clear();
			for (int i = 0; i < n_monsters; i++) {
				monsters.push_back(s->read_line());
			}
		} else if (type == "obj") {
			n_obj = s->read_int();
			obj.clear();
			for (int i = 0; i < n_obj; i++) {
				obj.push_back(s->read_line());
			}
		} else {
			// TODO: Throw more error info
			throw "NodeEventConfig: Invalid type";
		}
	}
};

struct NodeLinkConfig {
	std::string type;
	std::string name;
	
	NodeLinkConfig() {}
	
	NodeLinkConfig(Scanner *s) {
		type = s->read_string();
		name = s->read_line();
	}
};

struct NodeConfig {
	std::string name;
	
	NodeEventConfig event0;
	NodeEventConfig event1;
	NodeEventConfig event2;
	
	int n_links;
	std::vector<NodeLinkConfig> links;
	
	void init_links(Scanner *s) {
		n_links = s->read_int();
		
		links.clear();
		// TODO: Check links' validity
		for (int i = 0; i < n_links; i++) {
			links.push_back(NodeLinkConfig(s));
		}
	}
	
	NodeConfig(Scanner *s) {
		name = s->read_line();
		event0 = NodeEventConfig(s);
		event1 = NodeEventConfig(s);
		event2 = NodeEventConfig(s);
		
		init_links(s);
	}
};

struct Obj {
	std::string name;
	std::string type;
	int cnt;
	Obj(Scanner *s) {
		name = s->read_line();
		type = s->read_line();
		cnt = 0;
	}
};

struct Door {
	std::string name;
	std::string type;
	int cnt;
	Door(Scanner *s) {
		name = s->read_line();
		type = s->read_line();
		cnt = 0;
	}
};

struct TowerConfig {
	int init_HP, init_ATK, init_DEF, init_GOLD;
	
	int n_monsters;
	std::vector<MonsterConfig *> monsters;
	std::map<std::string, MonsterConfig *> monster_map;
	
	int n_nodes;
	std::vector<NodeConfig *> nodes;
	std::map<std::string, NodeConfig *> node_map;
	
	int n_obj;
	std::vector<Obj *> obj;
	std::map<std::string, Obj *> obj_map;
	
	int n_doors;
	std::vector<Door *> doors;
	std::map<std::string, Door *> door_map;
	
	void init_doors(Scanner *s) {
		n_doors = s->read_int();
		for (int i = 0; i< n_doors; ++i)
			doors.push_back(new Door(s));
			
		for (Door *conf : doors) {
			door_map[conf->name] = conf;
			door_map[conf->type] = conf;
		}
	}
	
	void init_obj(Scanner *s) {
		n_obj = s->read_int();
		for (int i = 0; i< n_obj; ++i)
			obj.push_back(new Obj(s));
			
		for (Obj *conf : obj) {
			obj_map[conf->name] = conf;
			obj_map[conf->type] = conf;
		}
	}
	
	void init_monsters(Scanner *s) {
		n_monsters = s->read_int();
		
		monsters.clear();
		for (int i = 0; i < n_monsters; i++) {
			monsters.push_back(new MonsterConfig(s));
		}
		
		// TODO: Check duplicate names
		for (MonsterConfig *conf : monsters) {
			monster_map[conf->name] = conf;
			monster_map[conf->short_name] = conf;
		}
	}
	
	void init_nodes(Scanner *s) {
		n_nodes = s->read_int();
		
		nodes.clear();
		for (int i = 0; i < n_nodes; i++) {
			nodes.push_back(new NodeConfig(s));
		}
		
		// TODO: Check duplicate names
		for (NodeConfig *conf : nodes) {
			node_map[conf->name] = conf;
		}
		
		// Rebuild links to bidirectional edges
		std::map<std::pair<std::string, std::string>, NodeLinkConfig> link_map;
		for (NodeConfig *node_conf : nodes) {
			for (NodeLinkConfig link_conf : node_conf->links) {
				std::string src_node = node_conf->name;
				std::string dest_node = link_conf.name;
				std::pair<std::string, std::string> p(src_node, dest_node);
				if (link_map.count(p) != 0) {
					continue;
				} else {
					link_map[p] = link_conf;
				}
				p = std::make_pair(dest_node, src_node);
				link_conf.name = src_node;
				link_map[p] = link_conf;
			}
			node_conf->links.clear();
			node_conf->n_links = 0;
		}
		for (auto p : link_map) {
			std::string src_node = p.first.first;
			std::string dest_node = p.first.second;
			if (node_map.count(src_node) == 0) {
				throw "Invalid node";
			}
			if (node_map.count(dest_node) == 0) {
				throw "Invalid node";
			}
			NodeConfig *n1 = node_map[src_node];
			NodeLinkConfig link_conf = p.second;
			++n1->n_links;
			n1->links.push_back(link_conf);
		}
	}
	
	TowerConfig(FILE *f) {
		Scanner *s = new Scanner(f);
		
		init_HP = s->read_int();
		init_ATK = s->read_int();
		init_DEF = s->read_int();
		init_GOLD = s->read_int();
		
		init_monsters(s);
		init_obj(s);
		init_doors(s);
		init_nodes(s);
		
		delete s;
	}
	
	std::string get_monster_full_name(std::string name) {
		if (monster_map.count(name) == 0) {
			// TODO: Throw more error info
			throw "Invalid monster name";
		}
		return monster_map[name]->name;
	}
	
	bool get_obj_status(std::string name) {
		if (obj_map.count(name) == 0)
			return false;
		++(obj_map[name]->cnt);
		return true;
	}
	
	bool get_doors_status(std::string name) {
		if (door_map.count(name) == 0)
			return false;
		++(door_map[name]->cnt);
		return true;
	}
	
	bool get_monster_status(std::string name, int &HP, int &ATK, int &DEF, int &GOLD) {
		if (monster_map.count(name) == 0) {
			return false;
		}
		MonsterConfig *conf = monster_map[name];
		HP = conf->HP;
		ATK = conf->ATK;
		DEF = conf->DEF;
		GOLD = conf->GOLD;
		return true;
	}
};

// ===========================
// Game Status

struct PlayerStatus {
	TowerConfig *config;
	
	int HP, ATK, DEF, GOLD;
	
	std::string fight_log,open_log;
	
	int n_obj;
	
	std::vector<Obj *> obj;
	
	PlayerStatus(TowerConfig *config) {
		this->config = config;
		HP = config->init_HP;
		ATK = config->init_ATK;
		DEF = config->init_DEF;
		GOLD = config->init_GOLD;
		n_obj = config->n_obj;
		for(auto i:config->obj)
			obj.push_back(i);
	}
	
	std::string get_status() {
		char buf[100];
		sprintf(buf, "HP: %d\nATK: %d\nDEF %d\nGOLD: %d\n\n", HP, ATK, DEF, GOLD);
		std::string w="";
		for(auto i:obj)
		if(i->name != "RedBottle" && i->name != "RB" && i->name != "BlueBottle" && i->name != "BB" && i->name != "RedStone" &&\
		 i->name != "RS" && i->name != "BlueStone" && i->name != "BS" && i->name != "Treasure" && i->name != "TS")
		{
			if(i->cnt>0)
			{
				char buf2[100];
				w=w+i->name;
				if(i->cnt!=1)
				{
					sprintf(buf2,"%d",i->cnt);
					std::string tmpp(buf2);
					w=w+"\t\t"+tmpp;
				}
				w+="\n";
			}
		}
		std::string ret(buf);
		ret=ret+w+"\n";
		if (HP == 0) {
			ret = "You were dead\n" + ret;
		}
		return ret;
	}
	
	int calc_damage(int a1, int d1, int h2, int a2, int d2) {
		if (a1 <= d2) {
			return -1;
		}
		if (a2 <= d1) {
			return 0;
		}
		return (h2 - 1) / (a1 - d2) * (a2 - d1);
	}
	void fight_monster(int &HP,int ATK,int DEF,int &mHP,int mATK,int mDEF) {
		int round = 0;
		char buf[1000];
		while(true)
		{
			sprintf(buf, "\nRound %d\n",++round);
			fight_log += buf;
			if(ATK<=mDEF)
				sprintf(buf, "You attacked the monster but nothing happend\n");
			else
			if(mHP>ATK-mDEF)
			{
				sprintf(buf, "Monster taken %d point(s) of damage.\nMonster's HP is %d now.\n",ATK-mDEF,mHP-ATK+mDEF);
				mHP-=ATK-mDEF;
			}
			else
			{
				sprintf(buf, "Monster taken %d point(s) of damage.\nMonster's HP is %d now.\n",mHP,0);
				mHP=0;
			}
			fight_log += buf;
			if(mHP==0)
				break;
			if(mATK<=DEF)
				sprintf(buf, "The monster attacked you but nothing happend\n");
			else
			if(HP>mATK-DEF)
			{
				sprintf(buf, "You taken %d point(s) of damage.\nYour HP is %d now.\n",mATK-DEF,HP-mATK+DEF);
				HP-=mATK-DEF;
			}
			else
			{
				sprintf(buf, "You taken %d point(s) of damage.\nYour HP is %d now.\n",HP,0);
				HP=0;
			}
			fight_log += buf;
			if(HP==0)
				break;
		}
	}
	
	Obj* get_obj(std::string name)
	{
		for (auto i:obj)
			if (i->name == name ||i->type == name)
				return i;
		return obj[0];
	}
	
	bool do_open(std::string name){
		if (!config->get_obj_status(name)) 
		if (!config->get_doors_status(name)) 
		{
			throw "Unknown obj error";
		}
		if (name == "RD" || name == "RedDoor" ||name == "YD" || \
		name == "YellowDoor" ||name == "BD" || name == "BlueDoor")
		{
			Obj* w = get_obj(name);
			if (w->cnt==0)
			{
				open_log = "No enough Key\n";
				return false;
			}
			--(w->cnt);
			open_log = "The Door Opened\n";
			
			return true;
		}
		open_log = "Pickup "+name+"\n";
		if (name == "RB" || name == "RedBottle")
		{
			open_log += "+ 100HP\n";
			HP += 100;
		}
		if (name == "BB" || name == "BlueBottle")
		{
			open_log += "+ 200HP\n";
			HP += 200;
		}
		if (name == "RS" || name == "RedStone")
		{
			open_log += "+ 1 ATK\n";
			ATK += 1;
		}
		if (name == "BS" || name == "BlueStone")
		{
			open_log += "+ 1 DEF\n";
			DEF += 1;
		}
		if (name == "TB" || name == "Treasure")
		{
			open_log += "+ 5 ATK\n+ 5 DEF\n+ 500 HP\n";
			ATK += 5;
			DEF += 5;
			HP += 500;
		}
		return true;
	}
	
	bool do_fight(std::string monster_name) {
		if (HP == 0) {
			fight_log = "You were dead, so you can't fight with anyone\n";
			return false;
		}
		
		fight_log = "Fighting with " + monster_name + " ...\n";
		fight_log += "Initial player status:\n";
		fight_log += get_status();
		fight_log += "\n";
		
		int mHP, mATK, mDEF, mGOLD;
		if (!config->get_monster_status(monster_name, mHP, mATK, mDEF, mGOLD)) {
			throw "Unknown error";
		}
		
		fight_log += "Monster status:\n";
		char buf[1000];
		sprintf(buf, "HP: %d\nATK: %d\nDEF %d\nGOLD: %d\n", mHP, mATK, mDEF, mGOLD);
		fight_log += std::string(buf);
		fight_log += "\n";
		
		int damage = calc_damage(ATK, DEF, mHP, mATK, mDEF);
		
		if (damage == -1) {
			fight_log += "Either you or the monster can make damage to each other.After played for 233 years...\n";
			fight_log += "You died\n";
			HP = 0;
			return false;
		}
		if (damage >= HP) {
			fight_monster(HP,ATK,DEF,mHP,mATK,mDEF);
			fight_log += "You have taken too much damage ...\n";
			fight_log += "You died\n";
			HP = 0;
			return false;
		} else {
			fight_monster(HP,ATK,DEF,mHP,mATK,mDEF);
			sprintf(buf, "You win!!!\nYou have taken %d points of damage in total.\n", damage);
			fight_log += std::string(buf);
			sprintf(buf, "You got %d GOLD\n", mGOLD);
			fight_log += std::string(buf);
			//HP -= damage;
			GOLD += mGOLD;
		}
		
		return true;
	}
	
	std::string get_fight_log() {
		return fight_log;
	}
	
	std::string get_open_log() {
		return open_log;
	}
	
};

struct NodeEventStatus {
	NodeEventConfig *config;
	TowerConfig *tower_config;
	PlayerStatus *player;
	
	std::string type;
	
	// Map monster_show_name -> monster_full_name
	std::map<std::string, std::string> alive_monsters;
	std::map<std::string, std::string> opend_doors;
	std::map<std::string, std::string> pickable_obj;
	
	NodeEventStatus(NodeEventConfig *config, TowerConfig *tower_config, PlayerStatus *player) {
		this->config = config;
		this->tower_config = tower_config;
		this->player = player;
		
		type = config->type;
		if (type == "door") {
			std::map<std::string, int> door_map;
			for (std::string name : config->doors) {
				//name = tower_config->get_monster_full_name(name);
				std::string show_name = name;
				int count = ++door_map[name];
				if (count > 1) {
					char buf[10];
					sprintf(buf, "%d", count);
					show_name += ' ';
					show_name += buf;
				}
				opend_doors[show_name] = name;
			}
		}
		if (type == "monster") {
			std::map<std::string, int> monster_count_map;
			for (std::string name : config->monsters) {
				name = tower_config->get_monster_full_name(name);
				std::string show_name = name;
				int count = ++monster_count_map[name];
				if (count > 1) {
					char buf[10];
					sprintf(buf, "%d", count);
					show_name += ' ';
					show_name += buf;
				}
				alive_monsters[show_name] = name;
			}
		}
		if (type == "obj") {
			std::map<std::string, int> obj_count_map;
			for (std::string name : config->obj) {
				//name = tower_config->get_monster_full_name(name);
				std::string show_name = name;
				int count = ++obj_count_map[name];
				if (count > 1) {
					char buf[10];
					sprintf(buf, "%d", count);
					show_name += ' ';
					show_name += buf;
				}
				pickable_obj[show_name] = name;
			}
		}
	}
	
	bool is_killed() {
		if (type == "none") {
			return true;
		} else if (type == "monster") {
			return alive_monsters.size() == 0;
		} else if (type == "door") {
			return opend_doors.size() == 0;
		} else {
			return false;
		}
	}
	
	bool is_picked() {
		if (type == "none") {
			return true;
		} else if (type == "obj") {
			return pickable_obj.size() == 0;
		} else if (type == "door") {
			return opend_doors.size() == 0;
		} else {
			return false;
		}
	}
	
	int do_read_file(std::string name, bool &can_exec, std::string &content) {
		if (type == "none") {
			return -ENOENT;
		} else if (type == "monster") {
			if (alive_monsters.count(name) == 0) {
				return -ENOENT;
			} else {
				content = "Don't touch me !!! I'll kill you !!!\n";
				can_exec = false;
				return 0;
			}
		} else if (type == "obj") {
			if (pickable_obj.count(name) == 0) {
				return -ENOENT;
			} else {
				content = "Maybe treasures.Pick it !\n";
				can_exec = false;
				return 0;
			}
		} else if (type == "door") {
			if (opend_doors.count(name) == 0) {
				return -ENOENT;
			} else {
				content = "Open the door with the right key.\n";
				can_exec = false;
				return 0;
			}
		}
	}
	
	int do_fight(std::string name) {
		if (type == "none") {
			return -EPERM;
		} else if (type == "monster") {
			if (alive_monsters.count(name) == 0) {
				return -EPERM;
			} else {
				std::string monster_name = alive_monsters[name];
				if (player->do_fight(monster_name)) {
					alive_monsters.erase(name);
				}
				return 0;
			}
		}
	}
	
	int do_open(std::string name) {
		if (type == "none") {
			return -EPERM;
		} else if (type == "obj") {
			/*if (name == ".all")
			{
				assert(0);
				for (auto &obj_name : pickable_obj)
					player->do_open(obj_name.second);
				pickable_obj.clear();
			}
			else */if (pickable_obj.count(name) == 0)
			{
				return -EPERM;
			}
			else
			{
				std::string obj_name = pickable_obj[name];
				if(player->do_open(obj_name)) {
					pickable_obj.erase(name);
				}
			}
			return 0;
		} else if (type == "door") {
			if (opend_doors.count(name) == 0)
			{
				return -EPERM;
			}
			else
			{
				std::string obj_name = opend_doors[name];
				if(player->do_open(obj_name)) {
					opend_doors.erase(name);
				}
			}
			return 0;
		}
	}
	
	void list_files(std::vector<std::string> &files) {
		if (type == "none") {
			return;
		} else if (type == "monster") {
			for (auto it : alive_monsters) {
				files.push_back(it.first);
			}
		} else if (type == "obj") {
			for (auto it : pickable_obj) {
				files.push_back(it.first);
			}
		} else if (type == "door") {
			for (auto it : opend_doors) {
				files.push_back(it.first);
			}
		}
	}
	
};

struct NodeLinkStatus {
	NodeLinkConfig *config;
	TowerConfig *tower_config;
	
	std::string type;
	std::string name;
	
	// TODO: Add status (e.g. doors)
	
	NodeLinkStatus(NodeLinkConfig *config, TowerConfig *tower_config) {
		this->config = config;
		this->tower_config = tower_config;
		
		type = config->type;
		name = config->name;
	}
	
	bool can_reach() {
		// TODO
		return true;
	}
};

struct NodeStatus {
	NodeConfig *config;
	TowerConfig *tower_config;
	PlayerStatus *player;
	
	std::string name;
	
	NodeEventStatus *event0;
	NodeEventStatus *event1;
	NodeEventStatus *event2;
	
	// Map link_show_name -> link_status
	std::map<std::string, NodeLinkStatus *> links;
	
	void init_links() {
		std::map<std::string, int> count_map;
		for (NodeLinkConfig &conf : config->links) {
			int cnt = ++count_map[conf.type];
			char buf[10];
			sprintf(buf, "%d", cnt);
			std::string link_show_name = conf.type + std::string(buf);
			links[link_show_name] = new NodeLinkStatus(&conf, tower_config);
		}
	}
	
	NodeStatus(NodeConfig *config, TowerConfig *tower_config, PlayerStatus *player) {
		this->config = config;
		this->tower_config = tower_config;
		this->player = player;
		
		name = config->name;
		event0 = new NodeEventStatus(&(config->event0), tower_config, player);
		event1 = new NodeEventStatus(&(config->event1), tower_config, player);
		event2 = new NodeEventStatus(&(config->event2), tower_config, player);
		
		init_links();
	}
	
	int walk(std::string name, std::string &next_node_name) {
		if (!event0->is_killed()) {
			return -ENOENT;
		}
		if (!event1->is_killed()) {
			return -ENOENT;
		}
		
		if (links.count(name) == 0) {
			return -ENOENT;
		}
		NodeLinkStatus *link = links[name];
		if (!link->can_reach()) {
			return -EPERM;
		}
		next_node_name = tower_config->node_map[link->name]->name;
		return 0;
	}
	
	// Monster or others
	int do_read_file(std::string name, bool &can_exec, std::string &content) {
		if (!event0->is_killed()) {
			int ret = event0->do_read_file(name, can_exec, content);
			return ret;
		} else if (!event1->is_killed()) {
			int ret = event1->do_read_file(name, can_exec, content);
			return ret;
		} else if (!event2->is_killed()) {
			int ret = event2->do_read_file(name, can_exec, content);
			return ret;
		} else {
			return -ENOENT;
		}
	}
	
	int do_fight(std::string name) {
		if (!event0->is_killed()) {
			return -ENOENT;
		} else if (!event1->is_killed()) {
			return event1->do_fight(name);
		} else {
			return -EPERM;
		}
	}
	
	int do_open(std::string name) {
		if (name == "RD" || name == "RedDoor" ||name == "YD" || \
		name == "YellowDoor" ||name == "BD" || name == "BlueDoor")
			return event0->do_open(name);
		return event2->do_open(name);
	}
	
	void list_files(std::vector<std::string> &files) {
		if (!event0->is_killed()) {
			event0->list_files(files);
		} else if (!event1->is_killed()) {
			event1->list_files(files);
		} else {
			if (!event2->is_killed()) {
				event2->list_files(files);
			}
			
			for (auto it : links) {
				// TODO: check for link permissions (?)
				files.push_back(it.first);
			}
		}
	}
	
};

struct TowerStatus {
	TowerConfig *config;
	
	PlayerStatus *player;
	
	std::vector<NodeStatus *> nodes;
	std::map<std::string, NodeStatus *> node_map;
	
	std::string readme_file;
	std::string fight_file;
	std::string open_file;
	
	std::string read_disk_file(const char *path) {
		FILE *f = fopen(path, "r");
		if (!f) {
			return "";
		}
		std::string ret;
		while (1) {
			char c = fgetc(f);
			if (c == EOF) {
				break;
			}
			ret += c;
		}
		fclose(f);
		return ret;
	}
	
	void init_nodes() {
		for (NodeConfig *node_conf : config->nodes) {
			NodeStatus *node_status = new NodeStatus(node_conf, config, player);
			nodes.push_back(node_status);
			node_map[node_conf->name] = node_status;
		}
	}
	
	TowerStatus(TowerConfig *config) {
		this->config = config;
		
		player = new PlayerStatus(config);
		
		init_nodes();
		
		readme_file = read_disk_file("tower-README.txt");
		fight_file = read_disk_file("tower-fight.txt");
		open_file = read_disk_file("tower-open.txt");
	}
	
	std::string get_status() {
		std::string ret = player->get_status();
		return ret;
	}
	
	int do_read_tool_file(std::string name, bool &can_exec, std::string &content) {
		if (name == ".README") {
			content = readme_file;
			can_exec = false;
			return 0;
		} else if (name == ".status") {
			content = get_status();
			can_exec = false;
			return 0;
		}  else if (name == ".all") {
			content = "";
			can_exec = false;
			return 0;
		} else if (name == ".fight_log") {
			content = player->get_fight_log();
			can_exec = false;
			return 0;
		} else if (name == "fight") {
			content = fight_file;
			can_exec = true;
			return 0;
		} else if (name == ".open_log") {
			content = player->get_open_log();
			can_exec = false;
			return 0;
		} else if (name == "pickup"||name == "open") {
			content = open_file;
			can_exec = true;
			return 0;
		} else {
			return -ENOENT;
		}
	}
	
	void list_tool_files(std::vector<std::string> &files) {
		files.push_back(".README");
		files.push_back(".status");
		files.push_back(".fight_log");
		files.push_back(".open_log");
		//files.push_back(".all");
	}
	
};

// ===========================
// Game Prensenting Layer

struct TowerGame {
	TowerConfig *config;
	TowerStatus *status;
	
	TowerGame(FILE *f) {
		config = new TowerConfig(f);
		status = new TowerStatus(config);
	}
	
	std::vector<std::string> split_path(std::string path) {
		std::vector<std::string> ret;
		std::string tmp;
		for (char s : path) {
			if (s == '/') {
				if (tmp.length() > 0) {
					ret.push_back(tmp);
					tmp = "";
				}
			} else {
				tmp += s;
			}
		}
		if (tmp.length() > 0) {
			ret.push_back(tmp);
		}
		return ret;
	}
	
	int do_read(std::string _path, bool &is_dir, bool &can_exec, std::string &content, std::vector<std::string> &files) {
		std::vector<std::string> path = split_path(_path);
		
		NodeStatus *node = status->nodes[0];
		int len_path = (int) path.size();
		
		// Check for files in root dir
		if (len_path == 1) {
			if (path[0] == "README") {
				is_dir = false;
				return status->do_read_tool_file(".README", can_exec, content);
			}
		}
		
		for (int i = 0; i < len_path; i++) {
			std::string name = path[i];
			NodeStatus *next_node;
			std::string next_node_name;
			int ret = node->walk(name, next_node_name);
			if (ret == 0) {
				node = status->node_map[next_node_name];
			} else if (ret == -ENOENT) {
				// Check for monsters and other events
				// Must be a file
				if (i + 1 != len_path) {
					return -ENOENT;
				}
				ret = node->do_read_file(path[i], can_exec, content);
				if (ret == 0) {
					is_dir = false;
					return 0;
				} else if (ret == -ENOENT) {
					// Check for tools
					ret = status->do_read_tool_file(path[i], can_exec, content);
					if (ret == 0) {
						is_dir = false;
						return 0;
					} else {
						return ret;
					}
				} else {
					return ret;
				}
			} else {
				return ret;
			}
		}
		
		is_dir = true;
		files.clear();
		node->list_files(files);
		
		// List tools
		status->list_tool_files(files);
		
		// Check for root dir
		if (path.size() == 0) {
			files.push_back("README");
		}
		
		return 0;
	}
	
	int getattr(const char *path, bool &is_dir, bool &can_exec, int &file_size) {
		std::string content;
		std::vector<std::string> files;
		int ret = do_read(std::string(path), is_dir, can_exec, content, files);
		if (ret != 0) {
			return ret;
		}
		if (!is_dir) {
			file_size = content.length();
		}
		return 0;
	}
	
	int readdir(const char *path, std::vector<std::string> &files) {
		bool is_dir, can_exec;
		std::string content;
		int ret = do_read(std::string(path), is_dir, can_exec, content, files);
		if (ret != 0) {
			return ret;
		}
		if (!is_dir) {
			return -ENOTDIR;
		}
		return 0;
	}
	
	int read(const char *path, char *buffer, int size, int offset) {
		bool is_dir, can_exec;
		std::string content;
		std::vector<std::string> files;
		int ret = do_read(std::string(path), is_dir, can_exec, content, files);
		if (ret != 0) {
			return ret;
		}
		if (is_dir) {
			return -EISDIR;
		}
		
		int len = content.length();
		if (offset < 0 || offset >= len) {
			return 0;
		}
		size = std::min(size, len - offset);
		memcpy(buffer, content.c_str() + offset, size);
		return size;
	}
	
	int write(const char *path, const char *buffer, int size, int offset) {
		std::string content(buffer, size);
		if (content == "open") {
			int ret = do_open(path);
			if (ret == 0) {
				ret = 5;
			}
			return ret;
		}
		if (content == "fight") {
			int ret = do_fight(path);
			if (ret == 0) {
				ret = 5;
			}
			return ret;
		} else {
			return -EPERM;
		}
	}
	
	int do_open(std::string _path) {
		std::vector<std::string> path = split_path(_path);
		
		NodeStatus *node = status->nodes[0];
		int len_path = (int) path.size();
		
		// Check for files in root dir
		if (len_path == 1) {
			if (path[0] == "README") {
				return -EPERM;
			}
		}
		
		for (int i = 0; i < len_path; i++) {
			std::string name = path[i];
			NodeStatus *next_node;
			std::string next_node_name;
			int ret = node->walk(name, next_node_name);
			if (ret == 0) {
				node = status->node_map[next_node_name];
			} else if (path[i]==".all") {
				ret = node->do_open(path[i]);
				return ret;
			} else if (ret == -ENOENT) {
				// Check for monsters and other events
				// Must be a file
				if (i + 1 != len_path) {
					return -ENOENT;
				}
				ret = node->do_open(path[i]);
				if (ret == 0) {
					return 0;
				} else if (ret == -ENOENT) {
					// Check for tools
					return -EPERM;
				} else {
					return ret;
				}
			} else {
				return ret;
			}
		}
		
		return -EPERM;
	}
	
	int do_fight(std::string _path) {
		std::vector<std::string> path = split_path(_path);
		
		NodeStatus *node = status->nodes[0];
		int len_path = (int) path.size();
		
		// Check for files in root dir
		if (len_path == 1) {
			if (path[0] == "README") {
				return -EPERM;
			}
		}
		
		for (int i = 0; i < len_path; i++) {
			std::string name = path[i];
			NodeStatus *next_node;
			std::string next_node_name;
			int ret = node->walk(name, next_node_name);
			if (ret == 0) {
				node = status->node_map[next_node_name];
			} else if (ret == -ENOENT) {
				// Check for monsters and other events
				// Must be a file
				if (i + 1 != len_path) {
					return -ENOENT;
				}
				ret = node->do_fight(path[i]);
				if (ret == 0) {
					return 0;
				} else if (ret == -ENOENT) {
					// Check for tools
					return -EPERM;
				} else {
					return ret;
				}
			} else {
				return ret;
			}
		}
		
		return -EPERM;
	}
	
};

static TowerGame *game;


static int do_getattr(const char *path, struct stat *st) {
	bool is_dir, can_exec;
	int file_size;
	int ret = game->getattr(path, is_dir, can_exec, file_size);
	if (ret != 0) {
		return ret;
	}
	
	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_atime = time(NULL);
	st->st_mtime = time(NULL);
	
	if (is_dir) {
		st->st_mode = S_IFDIR | 0555;
	} else {
		st->st_mode = S_IFREG | 0444;
		st->st_size = file_size;
		
		if (can_exec) {
			st->st_mode |= 0111;
		}
	}
	return 0;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	std::vector<std::string> files;
	int ret = game->readdir(path, files);
	
	if (ret != 0) {
		return ret;
	}
	
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);
	
	std::sort(files.begin(), files.end());
	
	for (std::string s : files) {
		filler(buffer, s.c_str(), NULL, 0);
	}
	
	return 0;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	return game->read(path, buffer, (int) size, (int) offset);
}

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	return game->write(path, buffer, (int) size, (int) offset);
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	return -EPERM;
}

static struct fuse_operations operations;

int main(int argc, char **argv) {
	FILE *f = fopen("tower.txt", "r");
	if (!f) {
		fprintf(stderr, "Error opening tower config file\n");
		return 1;
	}
	game = new TowerGame(f);
	fclose(f);
	
	operations.getattr = do_getattr;
	operations.readdir = do_readdir;
	operations.read = do_read;
	operations.write = do_write;
	operations.create = do_create;
	return fuse_main(argc, argv, &operations, NULL);
}
