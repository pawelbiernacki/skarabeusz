
#include "skarabeusz.h" 
#include <cstring>
#include <sstream>
#include <algorithm>
#include <fstream>

#define SKARABEUSZ_DEBUG

#ifdef SKARABEUSZ_DEBUG
#define DEBUG(X) std::cout << __LINE__ << ":" << X << "\n"
#else
#define DEBUG(X)
#endif

void skarabeusz::generator::run()
{
    target.resize(parameters.maze_x_range, parameters.maze_y_range, parameters.maze_z_range);
    target.generate_room_names();
    target.create_chambers(parameters.amount_of_chambers);
    target.choose_seed_rooms(*this);
    //target.grow_chambers_in_all_directions(*this);
    target.grow_chambers_horizontally(*this);
    target.grow_chambers_in_random_directions(*this);
    target.generate_door(*this);            
	generate_list_of_keys();            
    generate_list_of_pairs_chamber_and_keys();
    target.vector_of_virtual_door.clear();
    target.vector_of_virtual_door.resize(parameters.amount_of_chambers);	
	vector_of_equivalence_classes.clear();
	vector_of_equivalence_classes.resize(parameters.amount_of_chambers);
    distribute_the_pairs_into_equivalence_classes();
    copy_the_equivalence_classes_to_the_maze();
    process_the_equivalence_classes();
    generate_names();
    generate_paragraphs();
}   



void skarabeusz::generator::generate_names()
{
    for (unsigned chamber_id=1; chamber_id<=target.amount_of_chambers; chamber_id++)
    {
        target.vector_of_chambers[chamber_id-1]->set_guardian_name(get_random_name());
    }
}

const std::string skarabeusz::generator::names[]=
{
    "Noratdrok Lightarm",
    "Stroghem Lightbelt",
    "Gromdec Barrelblade",
    "Vorhirlum Nobleriver",
    "Yubrumir Merrygut",
    "Khurderlig Oakmail",
    "Saduc Ironbrow",
    "Hefrod Flintmace",
    "Thokkumlir Bronzebender",
    "Mundrig Hardcoat",
    "Haznorli Chainbane",
    "Doustreag Orefall",
    "Ruvolin Windsunder",
    "Destrumri Beastbeard",
    "Thedan Nightshoulder",
    "Kratmug Steelspine",
    "Tursoun Greyrock",
    "Olumlir Underhand",
    "Thatmorli Runebasher",
    "Kunorlun Strongbrew",
    "Thikdraic Leatherriver",
    "Norazmomi Grimaxe",
    "Hesuid Nightchin",
    "Rukhon Copperbrewer",
    "Wevril Honorgranite",
    "Thirog Flintgrip",
    "Durimrean Caskminer",
    "Gizzulim Leatherbuster",
    "Glorimaic Bloodbende"
};

std::string skarabeusz::generator::get_random_name()
{
    std::vector<std::string> temporary_vector_of_names;
    
    for (auto x:names)
    {
        if (map_name_to_taken_flag.find(x)==map_name_to_taken_flag.end())
        {
            temporary_vector_of_names.push_back(x);
        }
    }
    
    if (temporary_vector_of_names.size()==0)
    {
        throw std::runtime_error("not enough names");
    }
    
    std::uniform_int_distribution<unsigned> distr(0, temporary_vector_of_names.size()-1);            
    unsigned index = distr(get_random_number_generator());
    
    auto [it,success]=map_name_to_taken_flag.insert(std::pair(temporary_vector_of_names[index], true));
    if (!success)
    {
        throw std::runtime_error("failed to insert name");
    }
    
	return temporary_vector_of_names[index];
}




void skarabeusz::paragraph::print(std::ostream & s) const
{
    s << "\\begin{normalsize}\n";
	s << "\\textbf{\\hypertarget{par " << number << "}{" << number << "}}\n";
    
    for (auto & a: list_of_paragraph_items)
    {
        a->print(s);
        s << "\n";
    }
    s << "\\\\\n";    
    
    s << "\\end{normalsize}\n";
    s << "\\\\\n";
}


std::string skarabeusz::chamber::get_description(const maze & m) const
{
    std::stringstream result;
    std::string what = "";
    unsigned x1,y1,z1,x2,y2,z2;
    x1 = m.get_x1_of_chamber(id);
    y1 = m.get_y1_of_chamber(id);
    z1 = m.get_z1_of_chamber(id);
    x2 = m.get_x2_of_chamber(id);
    y2 = m.get_y2_of_chamber(id);
    z2 = m.get_z2_of_chamber(id);
    
    if (x2-x1==1 && y2-y1==1)
    {
        result << "Stoisz w małej komnacie.";
        what = "komnacie";
    }
    else
    if (x2-x1==0)
    {
        result << "Stoisz w korytarzu prowadzącym z północy na południe.";
        what = "korytarzu";
    }
    else
    if (y2-y1==0)
    {
        result << "Stoisz w korytarzu prowadzącym ze wschodu na zachód.";
        what = "korytarzu";
    }
    else
    if (x2-x1<=3 && y2-y1<=3)
    {
        if (x2-x1==y2-y1)
        {
            result << "Stoisz w niewielkiej kwadratowej komnacie.";
        }
        else
        {
            result << "Stoisz w niewielkiej komnacie.";
        }
        what = "komnacie";
    }
    else
    {
        if (x2-x1==y2-y1)
        {
            result << "Stoisz w dużej kwadratowej komnacie.";            
        }
        else
        {
            result << "Stoisz w dużej komnacie.";
        }
        what = "komnacie";
    }
    
    result << " Oprócz Ciebie w " << what << " jest jeszcze krasnolud na polu ";
    
    for (unsigned x=x1; x<=x2; x++)
    {
        for (unsigned y=y1; y<=y2; y++)
        {
            if (m.array_of_rooms[x][y][z1]->get_is_seed_room())
            {
                result << m.array_of_rooms[x][y][z1]->name << ". ";
            }
        }
    }
    
    result << m.get_wall_description(id, room::direction_type::NORTH)
            << m.get_wall_description(id, room::direction_type::EAST)
            << m.get_wall_description(id, room::direction_type::SOUTH)
            << m.get_wall_description(id, room::direction_type::WEST)
            << m.get_wall_description(id, room::direction_type::UP)
            << m.get_wall_description(id, room::direction_type::DOWN);
            
            
    return result.str();
}


bool skarabeusz::room::get_has_door_leading(direction_type d) const
{    
    return std::find_if(list_of_door.begin(),list_of_door.end(),[d](auto & x){ return x->get_direction()==d; })!=list_of_door.end();
}


std::string skarabeusz::maze::get_wall_description(unsigned id, room::direction_type d) const
{
    unsigned x1,y1,z1,x2,y2,z2;
    x1 = get_x1_of_chamber(id);
    y1 = get_y1_of_chamber(id);
    z1 = get_z1_of_chamber(id);
    x2 = get_x2_of_chamber(id);
    y2 = get_y2_of_chamber(id);
    z2 = get_z2_of_chamber(id);
    unsigned amount=0;
    std::stringstream result;
    std::string name;
    
    switch (d)
    {
        case room::direction_type::NORTH:
            for (unsigned x=x1;x<=x2; x++)
            {
                if (array_of_rooms[x][y1][z1]->get_has_door_leading(room::direction_type::NORTH))
                {
                    amount++;
                    name = array_of_rooms[x][y1][z1]->name;
                }
            }
            if (amount==0) return "";            
            
            if (amount == 1)
            {
                result << "W północnej ścianie są drzwi " << name << ". ";
            }
            else
            {
                result << "W północnej ścianie są " << amount << " pary drzwi. Są to ";
                for (unsigned x=x1;x<=x2; x++)
                {
                    if (array_of_rooms[x][y1][z1]->get_has_door_leading(room::direction_type::NORTH))
                    {
                        result << array_of_rooms[x][y1][z1]->name << " ";
                    }
                }                
                result << ". ";
            }
            break;
        case room::direction_type::EAST:
            for (unsigned y=y1;y<=y2; y++)
            {
                if (array_of_rooms[x2][y][z1]->get_has_door_leading(room::direction_type::EAST))
                {
                    amount++;
                    name = array_of_rooms[x2][y][z1]->name;
                }
            }            
            if (amount==0) return "";            
            
            if (amount == 1)
                result << "We wschodniej ścianie są drzwi " << name << ". ";
            else
            {
                result << "We wschodniej ścianie są " << amount << " pary drzwi. Są to ";
                for (unsigned y=y1;y<=y2; y++)
                {
                    if (array_of_rooms[x2][y][z1]->get_has_door_leading(room::direction_type::EAST))
                    {
                        result << array_of_rooms[x2][y][z1]->name << " ";
                    }
                }                            
                result << ". ";
            }
            break;
        case room::direction_type::SOUTH:
            for (unsigned x=x1;x<=x2; x++)
            {
                if (array_of_rooms[x][y2][z1]->get_has_door_leading(room::direction_type::SOUTH))
                {
                    amount++;
                    name = array_of_rooms[x][y2][z1]->name;
                }
            }
            if (amount==0) return "";
            
            if (amount == 1)
                result << "W południowej ścianie są drzwi " << name << ". ";
            else
            {
                result << "W południowej ścianie są " << amount << " pary drzwi. Są to ";
                for (unsigned x=x1;x<=x2; x++)
                {
                    if (array_of_rooms[x][y2][z1]->get_has_door_leading(room::direction_type::SOUTH))
                    {
                        result << array_of_rooms[x][y2][z1]->name << " ";                        
                    }
                }
                result << ". ";
            }
            break;
        case room::direction_type::WEST:
            for (unsigned y=y1;y<=y2; y++)
            {
                if (array_of_rooms[x1][y][z1]->get_has_door_leading(room::direction_type::WEST))
                {
                    amount++;
                    name = array_of_rooms[x1][y][z1]->name;
                }
            }                        
            if (amount==0) return "";            
            
            if (amount == 1)
                result << "W zachodniej ścianie są drzwi " << name << ". ";
            else
            {
                result << "W zachodniej ścianie są " << amount << " pary drzwi. ";
                for (unsigned y=y1;y<=y2; y++)
                {
                    if (array_of_rooms[x1][y][z1]->get_has_door_leading(room::direction_type::WEST))
                    {
                        result << array_of_rooms[x1][y][z1]->name << " ";
                    }
                }              
                result << ". ";
            }
            break;
        case room::direction_type::UP:
            for (unsigned x=x1;x<=x2; x++)
            {
                for (unsigned y=y1;y<=y2; y++)
                {
                    if (array_of_rooms[x][y][z2]->get_has_door_leading(room::direction_type::UP))
                    {
                        amount++;
                        name = array_of_rooms[x][y][z2]->name;
                    }
                }
            }                        
            if (amount==0) return "";            
            
            if (amount == 1)
                result << "W suficie jest właz " << name << ". ";
            else
            {
                result << "W suficie jest " << amount << " włazów. ";
                for (unsigned x=x1;x<=x2; x++)
                {
                    for (unsigned y=y1;y<=y2; y++)
                    {
                        if (array_of_rooms[x][y][z1]->get_has_door_leading(room::direction_type::UP))
                        {
                            result << array_of_rooms[x][y][z1]->name << " ";
                        }
                    }
                }
                result << ". ";
            }
            break;
        case room::direction_type::DOWN:
            for (unsigned x=x1;x<=x2; x++)
            {
                for (unsigned y=y1;y<=y2; y++)
                {
                    if (array_of_rooms[x][y][z1]->get_has_door_leading(room::direction_type::DOWN))
                    {
                        amount++;
                        name = array_of_rooms[x][y][z2]->name;
                    }
                }
            }                        
            if (amount==0) return "";            
            
            if (amount == 1)
                result << "W podłodze jest właz " << name << ". ";
            else
            {
                result << "W podłodze jest " << amount << " włazów. ";
                for (unsigned x=x1;x<=x2; x++)
                {
                    for (unsigned y=y1;y<=y2; y++)
                    {
                        if (array_of_rooms[x][y][z1]->get_has_door_leading(room::direction_type::DOWN))
                        {
                            result << array_of_rooms[x][y][z1]->name << " ";
                        }
                    }
                }
                result << ". ";
            }
            break;
    }    
    
    return result.str();
}


std::string skarabeusz::maze::get_keys_description_after_exchange(const keys & k) const
{
    std::stringstream s;
    
    unsigned amount=0;
    unsigned number=0,t;
    
    for (unsigned i=0; i<amount_of_chambers; i++)
    {
        for (unsigned j=i+1; j<amount_of_chambers; j++)
        {
            if (k.matrix[i][j]) { amount++; t=number+1; }
            number++;
        }
    }
    
    if (amount == 1)
    {
        s << "Krasnolud mówi - \"Chętnie dam Ci klucz " << t << " w zamian za Twoje klucze.\" ";
    }
    else
    {
        number=0;
        
        s << "Krasnolud mówi - \"Chętnie dam Ci klucze ";
        
        for (unsigned i=0; i<amount_of_chambers; i++)
        {
            for (unsigned j=i+1; j<amount_of_chambers; j++)
            {
                if (k.matrix[i][j]) { s << (number+1) << " "; }
                number++;
            }
        }
        s << " w zamian za Twoje klucze.\" ";
    }
    
    return s.str();
}

std::string skarabeusz::maze::get_keys_description(const keys & k) const
{
    std::stringstream s;
    unsigned amount=0;
    unsigned number=0,t;
    
    for (unsigned i=0; i<amount_of_chambers; i++)
    {
        for (unsigned j=i+1; j<amount_of_chambers; j++)
        {
            if (k.matrix[i][j]) { amount++; t=number+1; }
            number++;
        }
    }
    
    
    if (amount == 1)
    {
        s << "Masz przy sobie klucz numer " << t << ".";
    }
    else
    {
        s << "Masz przy sobie klucze o numerach: ";
        number=0;
        
        for (unsigned i=0; i<amount_of_chambers; i++)
        {
            for (unsigned j=i+1; j<amount_of_chambers; j++)
            {
                if (k.matrix[i][j]) { s << (number+1) << " "; }
                number++;
            }
        }
        s << ". ";
    }
    return s.str();
}

std::string skarabeusz::virtual_door::get_description(const maze & m, std::list<std::vector<std::vector<bool>>>::const_iterator x) const
{
    std::stringstream s;    
    keys k{*x};
    s << m.get_keys_description_after_exchange(k);    
    return s.str();
}


std::string skarabeusz::collection_of_virtual_door::get_description(const maze & m) const
{
    std::stringstream s;
    unsigned chamber_id;
    if (list_of_virtual_door.begin() == list_of_virtual_door.end())
    {
        s << "Krasnolud nie zwraca na Ciebie uwagi.\n";
    }
    else
    {
        chamber_id = list_of_virtual_door.begin()->chamber_id-1;
        
        DEBUG("collection_of_virtual_door::get_description chamber_id = " << chamber_id);
        DEBUG("x,y,z = " << m.vector_of_chambers[chamber_id]->seed.x << "," << m.vector_of_chambers[chamber_id]->seed.y << "," << m.vector_of_chambers[chamber_id]->seed.z);
        
        s << "Rozmawiasz z krasnoludem na polu " 
        << m.array_of_rooms[m.vector_of_chambers[chamber_id]->seed.x][m.vector_of_chambers[chamber_id]->seed.y][m.vector_of_chambers[chamber_id]->seed.z]->name << ". ";                
    }
    
    return s.str();
}

bool skarabeusz::room::get_door_can_be_opened_with(direction_type d, const keys & k) const
{
    for (auto &a: list_of_door)
    {
        if (a->direction == d && k.matrix[a->chamber1-1][a->chamber2-1])
        {
            return true;
        }
    }
    return false;
}


unsigned skarabeusz::maze::get_paragraph_index(unsigned x, unsigned y, unsigned z, const keys & k, paragraph::paragraph_type t) const
{
    for (unsigned i=0; i<vector_of_paragraphs.size(); i++)
    {
        if (vector_of_paragraphs[i]->my_state.my_keys == k && vector_of_paragraphs[i]->x == x && vector_of_paragraphs[i]->y == y && vector_of_paragraphs[i]->z == z && vector_of_paragraphs[i]->type == t)
        {
            return i;
        }
    }
    throw std::runtime_error("internal error");
}

unsigned skarabeusz::maze::get_paragraph_index(unsigned chamber_id, const keys & k, paragraph::paragraph_type t) const
{
    for (unsigned i=0; i<vector_of_paragraphs.size(); i++)
    {
        if (vector_of_paragraphs[i]->my_state.chamber_id == chamber_id && vector_of_paragraphs[i]->my_state.my_keys == k && vector_of_paragraphs[i]->type == t)
        {
            return i;
        }
    }
    throw std::runtime_error("internal error");    
}


void skarabeusz::generator::generate_paragraphs()
{
    unsigned number = 0;
    unsigned amount_of_chamber_descriptions = 0;
    unsigned amount_of_discussions = 0;
    unsigned amount_of_door_rooms = 0;
    unsigned amount_of_endings = 0;
    
    for (auto & i: list_of_processed_pairs_chamber_and_keys)
    {
        chamber_and_keys c{i.first, i.second};
        target.vector_of_paragraphs.push_back(std::make_unique<paragraph>(++number, c, paragraph::paragraph_type::CHAMBER_DESCRIPTION));
    }    
    
    amount_of_chamber_descriptions = list_of_processed_pairs_chamber_and_keys.size();
    
    for (auto & i: list_of_processed_pairs_chamber_and_keys)
    {
        chamber_and_keys c{i.first, i.second};
        target.vector_of_paragraphs.push_back(std::make_unique<paragraph>(++number, c, paragraph::paragraph_type::DISCUSSION));
    }    
    
    amount_of_discussions = list_of_processed_pairs_chamber_and_keys.size();
    
    do
    {
        std::uniform_int_distribution<unsigned> distr(0, amount_of_discussions-1);            
        unsigned index;
        
        do
        {
            index = distr(get_random_number_generator());
        }
        while (target.vector_of_paragraphs[index+amount_of_chamber_descriptions]->ending);
        
        target.vector_of_paragraphs[index+amount_of_chamber_descriptions]->ending = true;
        amount_of_endings++;        
    }
    while (amount_of_endings < parameters.amount_of_alternative_endings);
    
    

    for (auto & i: list_of_processed_pairs_chamber_and_keys)
    {
        chamber_and_keys c{i.first, i.second};
        unsigned x1,x2,y1,y2,z1,z2;
        unsigned chamber_id = i.first+1;
        x1 = target.get_x1_of_chamber(chamber_id);
        y1 = target.get_y1_of_chamber(chamber_id);
        z1 = target.get_z1_of_chamber(chamber_id);
        x2 = target.get_x2_of_chamber(chamber_id);
        y2 = target.get_y2_of_chamber(chamber_id);
        z2 = target.get_z2_of_chamber(chamber_id);
        
        for (unsigned x=x1; x<=x2; x++)
        {
            for (unsigned y=y1; y<=y2; y++)
            {
                for (unsigned z=z1; z<=z2; z++)
                {
                    if (target.array_of_rooms[x][y][z]->list_of_door.size()>0)
                    {
                        target.vector_of_paragraphs.push_back(std::make_unique<paragraph>(++number, c, x,y,z, paragraph::paragraph_type::DOOR));
                        amount_of_door_rooms++;
                    }
                }
            }
        }        
    }
  
    target.amount_of_paragraphs = number;
    
    for (unsigned i=0; i<amount_of_chamber_descriptions; i++)
    {
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.vector_of_chambers[target.vector_of_paragraphs[i]->my_state.chamber_id]->get_description(target)));
    }
    
    number=amount_of_chamber_descriptions + amount_of_discussions;
    
    for (unsigned i=0; i<amount_of_chamber_descriptions; i++)
    {
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.get_keys_description(target.vector_of_paragraphs[i]->my_state.my_keys)));
                
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>("Co chcesz zrobić? "));
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>("Porozmawiać z krasnoludem - idź do "));
        target.vector_of_paragraphs[i]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[i+amount_of_chamber_descriptions]));
        
        unsigned x1,x2,y1,y2,z1,z2;
        unsigned chamber_id = target.vector_of_paragraphs[i+amount_of_chamber_descriptions]->my_state.chamber_id+1;
        keys & k=target.vector_of_paragraphs[i+amount_of_chamber_descriptions]->my_state.my_keys;
        
        DEBUG("chamber_id " << chamber_id);

        x1 = target.get_x1_of_chamber(chamber_id);
        y1 = target.get_y1_of_chamber(chamber_id);
        z1 = target.get_z1_of_chamber(chamber_id);
        x2 = target.get_x2_of_chamber(chamber_id);
        y2 = target.get_y2_of_chamber(chamber_id);
        z2 = target.get_z2_of_chamber(chamber_id);

        for (unsigned x=x1; x<=x2; x++)
        {
            for (unsigned y=y1; y<=y2; y++)
            {
                for (unsigned z=z1; z<=z2; z++)
                {
                    if (target.array_of_rooms[x][y][z]->list_of_door.size()>0)
                    {                        
                        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>("Iść na pole "));
                        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.array_of_rooms[x][y][z]->name));                        
                        target.vector_of_paragraphs[i]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[number]));
                        
                        target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Stoisz przed drzwiami na polu "));
                        target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(target.array_of_rooms[x][y][z]->name));
                        target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(". "));                        
                        target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(target.get_keys_description(target.vector_of_paragraphs[i]->my_state.my_keys)));

                        for (unsigned d=0; d<6; d++)
                        {
                            if (target.array_of_rooms[x][y][z]->get_has_door_leading(static_cast<room::direction_type>(d)))
                            {
                                room::direction_type di=static_cast<room::direction_type>(d);
                                switch (di)
                                {
                                    case room::direction_type::NORTH:
                                        
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->my_state.my_keys))
                                        {                                        
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Możesz iść na północ "));                                                                                    
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x,y-1,z,k, paragraph::paragraph_type::DOOR)]));
                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Żaden z Twoich kluczy nie pasuje do północnych drzwi. "));
                                        }
                                        break;
                                    case room::direction_type::EAST:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->my_state.my_keys))
                                        {                                        
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Możesz iść na wschód "));
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x+1,y,z,k, paragraph::paragraph_type::DOOR)]));
                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Żaden z Twoich kluczy nie pasuje do wschodnich drzwi. "));
                                        }
                                        break;
                                    case room::direction_type::SOUTH:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->my_state.my_keys))
                                        {                                        
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Możesz iść na południe "));
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x,y+1,z,k, paragraph::paragraph_type::DOOR)]));                                            
                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Żaden z Twoich kluczy nie pasuje do południowych drzwi. "));
                                        }
                                        break;
                                    case room::direction_type::WEST:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->my_state.my_keys))
                                        {                                        
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Możesz iść na zachód "));
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x-1,y,z,k, paragraph::paragraph_type::DOOR)]));
                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Żaden z Twoich kluczy nie pasuje do zachodnich drzwi. "));
                                        }                                            
                                        break;
                                    case room::direction_type::DOWN:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->my_state.my_keys))
                                        {                                        
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Możesz zejść na niższy poziom "));
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x,y,z-1,k, paragraph::paragraph_type::DOOR)]));
                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Żaden z Twoich kluczy nie pasuje do klapy w podłodze. "));
                                        }
                                        break;
                                    case room::direction_type::UP:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->my_state.my_keys))
                                        {                                        
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Możesz wejść na wyższy poziom "));
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x,y,z+1,k, paragraph::paragraph_type::DOOR)]));
                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Żaden z Twoich kluczy nie pasuje do klapy w suficie. "));
                                        }
                                        break;                                        
                                }
                            }
                        }
                        
                        target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>("Możesz się rozejrzeć "));
                        target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(target.vector_of_paragraphs[number]->my_state.chamber_id,target.vector_of_paragraphs[number]->my_state.my_keys, paragraph::paragraph_type::CHAMBER_DESCRIPTION)]));
                                                
                        number++;                        
                    }
                }
            }
        }        
    }
    
    DEBUG("amount of chamber descriptions " << amount_of_chamber_descriptions);
    
    for (unsigned i=amount_of_chamber_descriptions; i<amount_of_chamber_descriptions+amount_of_discussions; i++)
    {
        DEBUG("get from chamber " << target.vector_of_paragraphs[i]->my_state.chamber_id);        
        unsigned chamber_id = target.vector_of_paragraphs[i]->my_state.chamber_id+1;
        keys & k=target.vector_of_paragraphs[i]->my_state.my_keys;
                
        auto it = std::find_if(target.vector_of_virtual_door[target.vector_of_paragraphs[i]->my_state.chamber_id].list_of_virtual_door.begin(),
                     target.vector_of_virtual_door[target.vector_of_paragraphs[i]->my_state.chamber_id].list_of_virtual_door.end(),
                     [&k](auto & x){return x.get_contains(k.matrix);});
        
        
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>("Stoisz obok krasnoluda na polu "));
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.array_of_rooms[target.vector_of_chambers[chamber_id-1]->seed.x][target.vector_of_chambers[chamber_id-1]->seed.y][target.vector_of_chambers[chamber_id-1]->seed.z]->name));
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(". "));                        
        
        
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.get_keys_description(target.vector_of_paragraphs[i]->my_state.my_keys)));
        
        bool exchange = false;
        
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>("Witaj, nazywam się "));
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.vector_of_chambers[chamber_id-1]->guardian_name));
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(". "));
        
        if (target.vector_of_paragraphs[i]->ending)
        {
            target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>("Gratulacje! Udało Ci się!"));
        }        
        else
        {
            if (it != target.vector_of_virtual_door[target.vector_of_paragraphs[i]->my_state.chamber_id].list_of_virtual_door.end())
            {
                for (auto x=it->list_of_matrices.begin(); x!=it->list_of_matrices.end(); x++)
                {
                    keys k2{*x};
                    if (k != k2)
                    {
                        std::string t = it->get_description(target, x);                
                        exchange=true;
                        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(t));
                        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>("Jeżeli się zgadzasz przejdź do "));                                        
                        target.vector_of_paragraphs[i]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(target.vector_of_paragraphs[i]->my_state.chamber_id, k2, paragraph::paragraph_type::DISCUSSION)]));
                        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(". "));
                    }
                }
            }
            if (!exchange)
            {
                target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>("Krasnolud nie zgadza się na żadną wymianę. "));            
            }
        }

        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>("Możesz się rozejrzeć "));
        target.vector_of_paragraphs[i]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(target.vector_of_paragraphs[i]->my_state.chamber_id,target.vector_of_paragraphs[i]->my_state.my_keys, paragraph::paragraph_type::CHAMBER_DESCRIPTION)]));        
    }
    
    std::shuffle(target.vector_of_paragraphs.begin(), target.vector_of_paragraphs.end(), gen);
    
    for (unsigned i=0; i<target.vector_of_paragraphs.size(); i++)
    {
        target.vector_of_paragraphs[i]->number = i+1;
    }
    
    
}


unsigned skarabeusz::generator::get_amount_of_pairs_that_have_been_done() const
{
    unsigned result = 0;
	for (int i=0; i<vector_of_pairs_chamber_and_keys.size(); i++)
	{
		if (vector_of_pairs_chamber_and_keys[i].get_done())
		{
            result++;
        }
    }
    return result;
}

bool skarabeusz::generator::get_all_pairs_have_been_done() const
{
	for (int i=0; i<vector_of_pairs_chamber_and_keys.size(); i++)
	{
		if (!vector_of_pairs_chamber_and_keys[i].get_done())
		{
			return false;
		}
	}
	return true;
}



bool skarabeusz::virtual_door::get_can_exchange(const std::vector<std::vector<bool> > & k1, const std::vector<std::vector<bool> > & k2) const
{
	return get_contains(k1) && get_contains(k2);
}

bool skarabeusz::virtual_door::get_contains(const std::vector<std::vector<bool> > & k) const
{
	for (std::list<std::vector<std::vector<bool>>>::const_iterator i(list_of_matrices.begin()); i != list_of_matrices.end(); i++)
	{
		if ((*i) == k)
			return true;
	}
	return false;
}


bool skarabeusz::generator::get_there_is_a_transition(const chamber_and_keys & p1, const chamber_and_keys & p2)
{
	// the keys are equal and there is a transition through a door
	if (p1.get_keys() == p2.get_keys() && p1.get_keys().matrix[p1.get_chamber_id()-1][p2.get_chamber_id()-1])
		return true;

	// the chambers are equal and the keys can be exchanged
	if (p1.get_chamber_id() == p2.get_chamber_id() && target.vector_of_virtual_door[p1.get_chamber_id()-1].get_can_exchange(p1.get_keys().matrix, p2.get_keys().matrix))
	{
		return true;
	}
	
	return false;
}


bool skarabeusz::collection_of_virtual_door::get_can_exchange(const std::vector<std::vector<bool>> & k1, const std::vector<std::vector<bool>> & k2) const
{
	for (std::list<virtual_door>::const_iterator i(list_of_virtual_door.begin()); i != list_of_virtual_door.end(); i++)
	{
		if ((*i).get_can_exchange(k1, k2))
			return true;
	}
	return false;
}

bool skarabeusz::generator::process_a_journey_step()
{
	bool iteration_done = false;
	for (unsigned i=0; i<vector_of_pairs_chamber_and_keys.size(); i++)
	{
		if (!vector_of_pairs_chamber_and_keys[i].get_done())
		{
			for (unsigned j=0; j<vector_of_pairs_chamber_and_keys.size(); j++)
			{
				if (vector_of_pairs_chamber_and_keys[j].get_done())
				{
					if (get_there_is_a_transition(vector_of_pairs_chamber_and_keys[i], vector_of_pairs_chamber_and_keys[j]))
					{
						vector_of_pairs_chamber_and_keys[i].set_done(true);
						iteration_done = true;
					}
				}
			}
		}
	}
	return iteration_done;
}


void skarabeusz::generator::process_a_journey_get_candidate(unsigned & candidate_done, unsigned & candidate_not_done) const
{
	for (unsigned i=0; i<vector_of_pairs_chamber_and_keys.size(); i++)
	{
		if (!vector_of_pairs_chamber_and_keys[i].get_done())
		{
			for (unsigned j=0; j<vector_of_pairs_chamber_and_keys.size(); j++)
			{
				if (vector_of_pairs_chamber_and_keys[j].get_done())
				{
					if (vector_of_pairs_chamber_and_keys[j].get_chamber_id() == vector_of_pairs_chamber_and_keys[i].get_chamber_id())
					{
						candidate_done = j;
						candidate_not_done = i;
						return;
					}
				}
			}
		}
	}
	
	throw std::runtime_error("internal error - unable to find a candidate for any chamber"); // should never happen
}

void skarabeusz::virtual_door::add_keys_from(const virtual_door & d)
{
	for (std::list<std::vector<std::vector<bool>>>::const_iterator i(d.list_of_matrices.begin()); i != d.list_of_matrices.end(); i++)
	{
		list_of_matrices.push_back(*i);
	}
}

void skarabeusz::collection_of_virtual_door::merge_classes(const std::vector<std::vector<bool> > & k1, const std::vector<std::vector<bool> > & k2)
{
	std::list<virtual_door>::iterator a = list_of_virtual_door.begin(), b = list_of_virtual_door.begin();

	// identify the classes containing the representants
	while (!(*a).get_contains(k1))
	{
		a++;
		if (a == list_of_virtual_door.end())
			throw std::runtime_error("keys not found - internal error");
	}

	while (!(*b).get_contains(k2))
	{
		b++;
		if (b == list_of_virtual_door.end())
			throw std::runtime_error("keys not found - internal error");
	}

	// create a class containing all the items
	virtual_door d{a->chamber_id};
	d.add_keys_from(*a);
	d.add_keys_from(*b);
        
	// remove the old classes
	list_of_virtual_door.erase(a);
	list_of_virtual_door.erase(b);

	// insert the new class
	list_of_virtual_door.push_back(d);
}

void skarabeusz::generator::process_a_journey(unsigned chamber1, keys & keys1, unsigned chamber2, keys & keys2)
{
    DEBUG("process a journey from " << chamber1 << " to " << chamber2);
    
	vector_of_pairs_chamber_and_keys.clear();
	for (std::list<std::pair<unsigned, keys>>::iterator i(list_of_processed_pairs_chamber_and_keys.begin()); i != list_of_processed_pairs_chamber_and_keys.end(); i++)
	{
		chamber_and_keys c((*i).first+1, (*i).second.matrix);
		vector_of_pairs_chamber_and_keys.push_back(c);
	}

	DEBUG("vector_of_pairs_chamber_and_keys.size() " << vector_of_pairs_chamber_and_keys.size());
    //keys2.report(std::cout);
    
	
	int target_index = 0;
	for (int i=0; i<vector_of_pairs_chamber_and_keys.size(); i++)
	{
		if (vector_of_pairs_chamber_and_keys[i].get_chamber_id() == chamber2 && vector_of_pairs_chamber_and_keys[i].get_keys() == keys2)
		{
			vector_of_pairs_chamber_and_keys[i].set_done(true);
            DEBUG("target index " << i);
			target_index = i;
		}
	}

	do
	{
        DEBUG("amount of pairs that have been done " << get_amount_of_pairs_that_have_been_done() << "/" << vector_of_pairs_chamber_and_keys.size());
        
		bool s = process_a_journey_step();

		if (!get_all_pairs_have_been_done() && !s)
		{
			// find a pair that has not been done, but a chamber that has
			unsigned candidate_not_done = 0, candidate_done = 0;

			process_a_journey_get_candidate(candidate_done, candidate_not_done);

			unsigned c = vector_of_pairs_chamber_and_keys[candidate_done].get_chamber_id();

			target.vector_of_virtual_door[c-1].merge_classes(vector_of_pairs_chamber_and_keys[candidate_not_done].get_keys().matrix, 
                                                                  vector_of_pairs_chamber_and_keys[candidate_done].get_keys().matrix);

			if (!process_a_journey_step())
				throw std::runtime_error("internal error");
		}
	}
	while (!get_all_pairs_have_been_done());
}



void skarabeusz::generator::process_the_equivalence_classes()
{
	for (std::list<std::pair<unsigned, keys>>::iterator i(list_of_processed_pairs_chamber_and_keys.begin()); i != list_of_processed_pairs_chamber_and_keys.end(); i++)
	{
		for (std::list<std::pair<unsigned, keys>>::iterator j(list_of_processed_pairs_chamber_and_keys.begin()); j != list_of_processed_pairs_chamber_and_keys.end(); j++)
		{
			if ((*i).first != (*j).first)
			{
				// if there is a direct connection between the chambers
				if (target.matrix_of_chamber_transitions[(*i).first][(*j).first])
				{
					process_a_journey((*i).first+1, (*i).second, (*j).first+1, (*j).second);
				}
			}
		}
	}
}


void skarabeusz::generator::copy_the_equivalence_classes_to_the_maze()
{
    target.vector_of_virtual_door.resize(target.amount_of_chambers);    
	for (unsigned i=1; i<=target.amount_of_chambers; i++)
	{
		for (std::list<std::list<keys>>::const_iterator j(vector_of_equivalence_classes[i-1].begin()); j != vector_of_equivalence_classes[i-1].end(); j++)
		{
			virtual_door d{i};
            
			for (std::list<keys>::const_iterator k((*j).begin()); k != (*j).end(); k++)
			{
				d.push_back((*k).matrix);
			}
			target.vector_of_virtual_door[i-1].push_back(d);
		}
	}
}



void skarabeusz::generator::add_pair_to_the_equivalence_classes(const std::pair<unsigned, keys> & p)
{
	std::list<keys> empty_list;
	empty_list.push_back(p.second);
	vector_of_equivalence_classes[p.first].push_back(empty_list); // create a new collection
}


void skarabeusz::generator::distribute_the_pairs_into_equivalence_classes()
{
	// add the first pair to the list of processed pairs
	list_of_processed_pairs_chamber_and_keys.clear();
	add_pair_to_the_equivalence_classes(*list_of_pairs_chamber_and_keys.begin());
	list_of_processed_pairs_chamber_and_keys.push_back(*list_of_pairs_chamber_and_keys.begin());
	list_of_pairs_chamber_and_keys.erase(list_of_pairs_chamber_and_keys.begin());



	// add subsequent pairs
	while (list_of_pairs_chamber_and_keys.size()>0)
	{
		bool no_need_to_connect;
		std::list<std::pair<unsigned, keys>>::iterator i = get_next_pair_to_connect(no_need_to_connect);
		add_and_connect_pair_to_the_equivalence_classes(*i, no_need_to_connect);
		list_of_processed_pairs_chamber_and_keys.push_back(*i);
		list_of_pairs_chamber_and_keys.erase(i);
	}    
}

void skarabeusz::generator::add_and_connect_pair_to_the_equivalence_classes(const std::pair<unsigned, keys> & p, bool no_need_to_connect)
{
	if (no_need_to_connect)
	{
		std::list<keys> empty_list;
		empty_list.push_back(p.second);

		vector_of_equivalence_classes[p.first].push_front(empty_list); // create a new collection
	}
	else
	{
		(*vector_of_equivalence_classes[p.first].begin()).push_back(p.second);
	}
}


std::list<std::pair<unsigned, skarabeusz::keys> >::iterator skarabeusz::generator::get_next_pair_to_connect_1()
{
	std::vector<std::list<std::pair<unsigned, keys>>::iterator> vector_of_iterators;

	for (std::list<std::pair<unsigned,keys>>::const_iterator i(list_of_processed_pairs_chamber_and_keys.begin()); i != list_of_processed_pairs_chamber_and_keys.end(); i++)
	{
		for (std::list<std::pair<unsigned, keys>>::iterator j(list_of_pairs_chamber_and_keys.begin()); j != list_of_pairs_chamber_and_keys.end(); j++)
		{
			if ((*i).second == (*j).second)
			{
				vector_of_iterators.push_back(j);
			}
		}
	}
	
    std::uniform_int_distribution<unsigned> distr(0, vector_of_iterators.size()-1);            
    unsigned index = distr(get_random_number_generator());
	return vector_of_iterators[index];
}

std::list<std::pair<unsigned, skarabeusz::keys>>::iterator skarabeusz::generator::get_next_pair_to_connect_2()
{
	std::vector<std::list<std::pair<unsigned, keys>>::iterator> vector_of_iterators;

	for (std::list<std::pair<unsigned, keys>>::const_iterator i(list_of_processed_pairs_chamber_and_keys.begin()); i != list_of_processed_pairs_chamber_and_keys.end(); i++)
	{
		for (std::list<std::pair<unsigned, keys>>::iterator j(list_of_pairs_chamber_and_keys.begin()); j != list_of_pairs_chamber_and_keys.end(); j++)
		{
			if ((*i).first == (*j).first)
			{
				vector_of_iterators.push_back(j);
			}
		}
	}
    std::uniform_int_distribution<unsigned> distr(0, vector_of_iterators.size()-1);            
    unsigned index = distr(get_random_number_generator());
	return vector_of_iterators[index];
}




std::list<std::pair<unsigned, skarabeusz::keys>>::iterator skarabeusz::generator::get_next_pair_to_connect(bool & no_need_to_connect)
{
	// try to find a different chamber for the already processed set of keys
	for (std::list<std::pair<unsigned, keys>>::const_iterator i(list_of_processed_pairs_chamber_and_keys.begin()); i != list_of_processed_pairs_chamber_and_keys.end(); i++)
	{
		for (std::list<std::pair<unsigned, keys>>::iterator j(list_of_pairs_chamber_and_keys.begin()); j != list_of_pairs_chamber_and_keys.end(); j++)
		{
			if ((*i).second == (*j).second)
			{
				no_need_to_connect = true;
				return get_next_pair_to_connect_1(); // no need to connect
			}
		}
	}

	// else try to find an already processed chamber - and connect
	for (std::list<std::pair<unsigned, keys>>::const_iterator i(list_of_processed_pairs_chamber_and_keys.begin()); i != list_of_processed_pairs_chamber_and_keys.end(); i++)
	{
		for (std::list<std::pair<unsigned, keys> >::iterator j(list_of_pairs_chamber_and_keys.begin()); j != list_of_pairs_chamber_and_keys.end(); j++)
		{
			if ((*i).first == (*j).first)
			{
				no_need_to_connect = false;
				return get_next_pair_to_connect_2(); // needs to be connected
			}
		}
	}

	throw std::runtime_error("generator internal error"); // should never happen
}



void skarabeusz::keys::report(std::ostream & s) const
{
    for (unsigned a=0; a<matrix.size(); a++)
    {
        for (unsigned b=0; b<matrix[a].size(); b++)
        {
            if (matrix[a][b])
            {
                s << "1";
            }
            else
            {
                s << "0";
            }
        }
        s << "\n";
    }    
}

bool skarabeusz::keys::operator==(const keys & other) const
{
    for (unsigned a=0; a<matrix.size(); a++)
    {
        for (unsigned b=0; b<matrix[a].size(); b++)
        {
            if (matrix[a][b]!=other.matrix[a][b])
            {
                return false;
            }
        }
    }
    return true;
}

void skarabeusz::generator::generate_list_of_keys()
{
    list_of_keys.clear();
    
    for (unsigned chamber1=1; chamber1<=target.amount_of_chambers; chamber1++)
    {
        for (unsigned chamber2=1; chamber2<=target.amount_of_chambers; chamber2++)
        {
            if (chamber1!=chamber2)
            {
                keys k{target};
                
                DEBUG("checking " << chamber1 << " to " << chamber2);
                                                
                if (k.matrix[chamber1-1][chamber2-1])
                {
                    keys k2{k};
                    DEBUG("chamber " << chamber1 << " to " << chamber2);
                    
                    if (k.get_amount_of_keys() > parameters.max_amount_of_keys_to_hold)
                    {                        
                        unsigned amount_of_keys_to_select=parameters.max_amount_of_keys_to_hold;
                        std::vector<std::pair<unsigned,unsigned>> keys_to_select;
                    
                        for (unsigned a=1;a<=target.amount_of_chambers; a++)
                        {
                            for (unsigned b=a+1;b<=target.amount_of_chambers;b++)
                            {
                                if (k2.matrix[a-1][b-1])
                                {
                                    keys_to_select.push_back(std::pair(a-1,b-1));
                                }
                            }
                        }
                        
                        std::vector<unsigned> vector_of_indeces_to_select;
                        vector_of_indeces_to_select.resize(parameters.max_amount_of_keys_to_hold);
                        for (unsigned i=0; i<parameters.max_amount_of_keys_to_hold; i++)
                        {
                            vector_of_indeces_to_select[0]=0;
                        }
                        
                        DEBUG("amount of keys to select " << amount_of_keys_to_select << ", keys_to_select.size() " << keys_to_select.size());
                        
                        
                        bool finished=false;
                        do
                        {
                            k2=false;
                            
                            for (unsigned i=0; i<amount_of_keys_to_select; i++)
                            {
                                auto [a,b]=keys_to_select[vector_of_indeces_to_select[i]];
                                k2.matrix[a][b]=true;
                                k2.matrix[b][a]=true;
                            }
                            
                            //k2.report(std::cout);
                            
                            if (std::find(list_of_keys.begin(),list_of_keys.end(), k2)==list_of_keys.end())
                            {
                                if (k2.get_amount_of_keys() != 0 && k2.get_amount_of_keys()<=parameters.max_amount_of_keys_to_hold && k2.matrix[chamber1-1][chamber2-1])
                                {
                                    list_of_keys.push_back(k2);
                                    DEBUG("add a combination of keys");
                                }
                                else
                                {
                                    DEBUG("could not add a combination, amount of keys " << k2.get_amount_of_keys() << " transition from " << chamber1 << " to " << chamber2 << " equals " << (k2.matrix[chamber1-1][chamber2-1] ? "1" : "0"));
                                }
                            }
                            else
                            {
                                DEBUG("it is there already");
                            }
                            
                            
                            for (unsigned i=0;i<amount_of_keys_to_select; i++)
                            {                                
                                DEBUG("incrementing index " << i << "(value " << vector_of_indeces_to_select[i] << ")");
                                vector_of_indeces_to_select[i]++;
                                if (vector_of_indeces_to_select[i]<keys_to_select.size())
                                {
                                    DEBUG("it is still smaller than " << keys_to_select.size());
                                    break;
                                }
                                else
                                {
                                    if (i+1==amount_of_keys_to_select)
                                    {
                                        finished = true;
                                        DEBUG("finished!");
                                        break;
                                    }
                                    else
                                    {
                                        vector_of_indeces_to_select[i]=0;
                                        DEBUG("set to 0 and continue");
                                        continue;
                                    }
                                }
                            }                            
                            
                            DEBUG("continue");
                        }
                        while (!finished);
                    }
                    else
                    {
                        if (std::find(list_of_keys.begin(),list_of_keys.end(), k2)!=list_of_keys.end())
                        {
                            continue;
                        }
                        if (k2.get_amount_of_keys() == 0)
                        {
                            continue;
                        }
                        list_of_keys.push_back(k2);                        
                        DEBUG("add a combination of keys");
                    }                    
                }
            }
        }
    }
    
    DEBUG("list of keys contains " << list_of_keys.size() << " items");
    
    /*
    for (auto & k: list_of_keys)
    {
        k.report(std::cout);
        std::cout << "\n";
    }
    */
}


skarabeusz::keys& skarabeusz::keys::operator=(bool v)
{
	for (int x=0; x<matrix.size(); x++)
	{
		for (int y=0; y<matrix[x].size(); y++)
			matrix[x][y] = v;
    }
    return *this;
}

unsigned skarabeusz::keys::get_amount_of_keys() const
{
	unsigned result = 0;

	for (int x=0; x<matrix.size(); x++)
	{
		for (int y=0; y<matrix[x].size(); y++)
			if (x!=y && matrix[x][y])
				result ++;
	}

	return result/2;
}


bool skarabeusz::keys::get_can_exit_chamber(unsigned f) const
{
	for (unsigned i=0; i<matrix.size(); i++)
	{
		if (matrix[f-1][i])
			return true;
	}
	return false;
}

void skarabeusz::generator::generate_list_of_pairs_chamber_and_keys()
{
	list_of_pairs_chamber_and_keys.clear();
	for (auto& i: list_of_keys)
	{
		for (unsigned j=1; j<=target.amount_of_chambers; j++)
		{
			if (i.get_can_exit_chamber(j))
			{
				list_of_pairs_chamber_and_keys.push_back(std::pair(j-1, i));
                
                DEBUG("add a pair for chamber and keys " << j);
                //i.report(std::cout);
			}
		}
	}
}


void skarabeusz::maze::generate_door(generator & g)
{
    matrix_of_chamber_transitions.resize(amount_of_chambers);
    for (unsigned chamber1=1;chamber1<=amount_of_chambers; chamber1++)
    {
        matrix_of_chamber_transitions[chamber1-1].resize(amount_of_chambers);
        for (unsigned chamber2=1;chamber2<=amount_of_chambers; chamber2++)
        {
            matrix_of_chamber_transitions[chamber1-1][chamber2-1]=false;
        }        
    }
    
    
    for (unsigned chamber1=1;chamber1<amount_of_chambers; chamber1++)
    {
        for (unsigned chamber2=chamber1+1;chamber2<=amount_of_chambers; chamber2++)
        {
            std::vector<std::shared_ptr<room>> v1,v2;
            
            std::vector<std::pair<std::shared_ptr<room>,std::shared_ptr<room>>> temporary_vector_of_door_candidates;
            std::vector<std::pair<room::direction_type,room::direction_type>> temporary_vector_of_directions;
            
            get_all_rooms_of_chamber(chamber1, v1);
            DEBUG("checking adjacent rooms between " << chamber1 << " and " << chamber2);
            
            for (auto & r1: v1)
            {
                unsigned x1=r1->x,y1=r1->y,z1=r1->z;
                for (unsigned d=0; d<6; d++)
                {
                    switch (static_cast<room::direction_type>(d))
                    {
                        case room::direction_type::NORTH:
                            if (y1 > 0)
                            {
                                if (array_of_rooms[x1][y1-1][z1]->chamber_id==chamber2)
                                {
                                    auto r1prime{r1};
                                    auto r2prime{array_of_rooms[x1][y1-1][z1]};
                                    temporary_vector_of_door_candidates.push_back(std::pair(std::move(r1prime),std::move(r2prime)));
                                    temporary_vector_of_directions.push_back(std::pair(room::direction_type::NORTH, room::direction_type::SOUTH));
                                }
                            }
                            break;
                            
                        case room::direction_type::EAST:
                            if (x1 < max_x_range-1)
                            {
                                if (array_of_rooms[x1+1][y1][z1]->chamber_id==chamber2)
                                {
                                    auto r1prime{r1};
                                    auto r2prime{array_of_rooms[x1+1][y1][z1]};
                                    temporary_vector_of_door_candidates.push_back(std::pair(std::move(r1prime),std::move(r2prime)));
                                    temporary_vector_of_directions.push_back(std::pair(room::direction_type::EAST, room::direction_type::WEST));
                                }
                            }
                            break;
                            
                        case room::direction_type::SOUTH:
                            if (y1 < max_y_range-1)
                            {
                                if (array_of_rooms[x1][y1+1][z1]->chamber_id==chamber2)
                                {
                                    auto r1prime{r1};
                                    auto r2prime{array_of_rooms[x1][y1+1][z1]};
                                    temporary_vector_of_door_candidates.push_back(std::pair(std::move(r1prime),std::move(r2prime)));
                                    temporary_vector_of_directions.push_back(std::pair(room::direction_type::SOUTH, room::direction_type::NORTH));
                                }
                            }
                            break;
                            
                        case room::direction_type::WEST:
                            if (x1 > 0)
                            {
                                if (array_of_rooms[x1-1][y1][z1]->chamber_id==chamber2)
                                {
                                    auto r1prime{r1};
                                    auto r2prime{array_of_rooms[x1-1][y1][z1]};
                                    temporary_vector_of_door_candidates.push_back(std::pair(std::move(r1prime),std::move(r2prime)));
                                    temporary_vector_of_directions.push_back(std::pair(room::direction_type::WEST, room::direction_type::EAST));
                                }
                            }
                            break;
                            
                        case room::direction_type::UP:
                            if (z1 < max_z_range-1)
                            {
                                if (array_of_rooms[x1][y1][z1+1]->chamber_id==chamber2)
                                {
                                    auto r1prime{r1};
                                    auto r2prime{array_of_rooms[x1][y1][z1+1]};
                                    temporary_vector_of_door_candidates.push_back(std::pair(std::move(r1prime),std::move(r2prime)));
                                    temporary_vector_of_directions.push_back(std::pair(room::direction_type::UP, room::direction_type::DOWN));
                                }
                            }
                            break;
                            
                        case room::direction_type::DOWN:
                            if (z1 > 0)
                            {
                                if (array_of_rooms[x1][y1][z1-1]->chamber_id==chamber2)
                                {
                                    auto r1prime{r1};
                                    auto r2prime{array_of_rooms[x1][y1][z1-1]};
                                    temporary_vector_of_door_candidates.push_back(std::pair(std::move(r1prime),std::move(r2prime)));
                                    temporary_vector_of_directions.push_back(std::pair(room::direction_type::DOWN, room::direction_type::UP));
                                }
                            }
                            break;
                    }
                }
            }
            
            DEBUG("for " << chamber1 << " and " << chamber2 << " got "
                << temporary_vector_of_door_candidates.size() << " door candidates");
            
            if (temporary_vector_of_door_candidates.size()>0)
            {
                std::uniform_int_distribution<unsigned> distr(0, temporary_vector_of_door_candidates.size()-1);
            
                unsigned index = distr(g.get_random_number_generator());
            
                DEBUG("selecting candidate " << index);
                
                auto &r1{temporary_vector_of_door_candidates[index].first};
                auto &r2{temporary_vector_of_door_candidates[index].second};

                r1->list_of_door.push_back(std::make_shared<door>(r1->chamber_id, r2->chamber_id, *r2, temporary_vector_of_directions[index].first));
                r2->list_of_door.push_back(std::make_shared<door>(r2->chamber_id, r1->chamber_id, *r1, temporary_vector_of_directions[index].second));
                
                matrix_of_chamber_transitions[r1->chamber_id-1][r2->chamber_id-1]=true;
                matrix_of_chamber_transitions[r2->chamber_id-1][r1->chamber_id-1]=true;
                amount_of_keys++;
            }
        }
    }
}

skarabeusz::keys::keys(const maze & m)
{
    matrix.resize(m.amount_of_chambers);
    for (unsigned chamber1=1; chamber1<=m.amount_of_chambers; chamber1++)
    {
        matrix[chamber1-1].resize(m.amount_of_chambers);
        for (unsigned chamber2=1; chamber2<=m.amount_of_chambers; chamber2++)
        {
            matrix[chamber1-1][chamber2-1] = m.matrix_of_chamber_transitions[chamber1-1][chamber2-1];
        }
    }
}

void skarabeusz::maze::generate_room_names()
{
    for (unsigned x=0; x<max_x_range; x++)
    {
        for (unsigned y=0; y<max_y_range; y++)
        {
            for (unsigned z=0; z<max_z_range; z++)
            {
                std::stringstream s;
                s << static_cast<char>('A'+x) 
                << static_cast<char>('A'+y) 
                << static_cast<char>('A'+z);
                array_of_rooms[x][y][z]->set_name(s.str());
            }
        }
    }

}


void skarabeusz::room::set_name(const std::string & n)
{
    name = n;
}

void skarabeusz::maze::grow_chambers_horizontally(generator & g)
{
    std::vector<std::shared_ptr<chamber>> temporary_vector;
    do
    {
        temporary_vector.clear();
        get_all_chambers_that_can_grow_horizontally(temporary_vector);
        if (temporary_vector.size() != 0)
        {
            std::uniform_int_distribution<unsigned> distr(0, temporary_vector.size()-1);
            
            unsigned index = distr(g.get_random_number_generator());
            
            temporary_vector[index]->grow_in_direction(room::direction_type::NORTH, *this);
            temporary_vector[index]->grow_in_direction(room::direction_type::EAST, *this);
            temporary_vector[index]->grow_in_direction(room::direction_type::SOUTH, *this);
            temporary_vector[index]->grow_in_direction(room::direction_type::WEST, *this);
        }
    }
    while (temporary_vector.size()>0);
}

void skarabeusz::maze::grow_chambers_in_random_directions(generator & g)
{
    std::vector<std::pair<std::shared_ptr<chamber>,room::direction_type>> temporary_vector;
    
    do
    {
        temporary_vector.clear();
        get_all_chambers_that_can_grow_in_random_directions(temporary_vector);
        if (temporary_vector.size() != 0)
        {
            std::uniform_int_distribution<unsigned> distr(0, temporary_vector.size()-1);
            
            unsigned index = distr(g.get_random_number_generator());
            temporary_vector[index].first->grow_in_direction(temporary_vector[index].second, *this);
        }
    }
    while (temporary_vector.size()!=0);
    
}


void skarabeusz::maze::grow_chambers_in_all_directions(generator & g)
{
    std::vector<std::shared_ptr<chamber>> temporary_vector;
    do
    {
        temporary_vector.clear();
        get_all_chambers_that_can_grow_in_all_directions(temporary_vector);
        //std::cout << "amount of chambers that can grow in all directions "
        //    << temporary_vector.size() << "\n";
        if (temporary_vector.size() != 0)
        {
            std::uniform_int_distribution<unsigned> distr(0, temporary_vector.size()-1);
            
            unsigned index = distr(g.get_random_number_generator());
            
            //std::cout << "grow chamber " << temporary_vector[index]->id << " in all directions\n";
            
            temporary_vector[index]->grow_in_all_directions(*this);
        }
    }
    while (temporary_vector.size()!=0);
}



void skarabeusz::chamber::grow_in_all_directions(maze & m) const
{
    unsigned x1,y1,x2,y2,z1,z2;
    x1 = m.get_x1_of_chamber(id);
    y1 = m.get_y1_of_chamber(id);
    z1 = m.get_z1_of_chamber(id);
    x2 = m.get_x2_of_chamber(id);
    y2 = m.get_y2_of_chamber(id);
    z2 = m.get_z2_of_chamber(id);
    
    for (unsigned x=x1-1;x<=x2+1; x++)
    {
        for (unsigned y=y1-1;y<=y2+1; y++)
        {
            for (unsigned z=z1-1;z<=z2+1; z++)
            {
                if (m.array_of_rooms[x][y][z]->get_chamber_id()==0)
                    m.array_of_rooms[x][y][z]->assign_to_chamber(id);
            }
        }
    }
}


void skarabeusz::chamber::grow_in_direction(room::direction_type d, maze & m) const
{
    unsigned x1,y1,x2,y2,z1,z2;
    x1 = m.get_x1_of_chamber(id);
    y1 = m.get_y1_of_chamber(id);
    z1 = m.get_z1_of_chamber(id);
    x2 = m.get_x2_of_chamber(id);
    y2 = m.get_y2_of_chamber(id);
    z2 = m.get_z2_of_chamber(id);

    switch (d)
    {
        case room::direction_type::NORTH:
            for (unsigned x=x1;x<=x2; x++)
            {
                for (unsigned y=y1-1;y<=y2; y++)
                {
                    for (unsigned z=z1;z<=z2; z++)
                    {
                        if (m.array_of_rooms[x][y][z]->get_chamber_id()==0)
                            m.array_of_rooms[x][y][z]->assign_to_chamber(id);
                    }
                }
            }
            break;
            
        case room::direction_type::EAST:
            for (unsigned x=x1;x<=x2+1; x++)
            {
                for (unsigned y=y1;y<=y2; y++)
                {
                    for (unsigned z=z1;z<=z2; z++)
                    {
                        if (m.array_of_rooms[x][y][z]->get_chamber_id()==0)
                            m.array_of_rooms[x][y][z]->assign_to_chamber(id);
                    }
                }
            }
            break;
            
        case room::direction_type::SOUTH:
            for (unsigned x=x1;x<=x2; x++)
            {
                for (unsigned y=y1;y<=y2+1; y++)
                {
                    for (unsigned z=z1;z<=z2; z++)
                    {
                        if (m.array_of_rooms[x][y][z]->get_chamber_id()==0)
                            m.array_of_rooms[x][y][z]->assign_to_chamber(id);
                    }
                }
            }
            break;
            
        case room::direction_type::WEST:
            for (unsigned x=x1-1;x<=x2; x++)
            {
                for (unsigned y=y1;y<=y2; y++)
                {
                    for (unsigned z=z1;z<=z2; z++)
                    {
                        if (m.array_of_rooms[x][y][z]->get_chamber_id()==0)
                            m.array_of_rooms[x][y][z]->assign_to_chamber(id);
                    }
                }
            }
            break;
            
        case room::direction_type::UP:
            for (unsigned x=x1;x<=x2; x++)
            {
                for (unsigned y=y1;y<=y2; y++)
                {
                    for (unsigned z=z1;z<=z2+1; z++)
                    {
                        if (m.array_of_rooms[x][y][z]->get_chamber_id()==0)
                            m.array_of_rooms[x][y][z]->assign_to_chamber(id);
                    }
                }
            }
            break;
            
        case room::direction_type::DOWN:
            for (unsigned x=x1;x<=x2; x++)
            {
                for (unsigned y=y1;y<=y2; y++)
                {
                    for (unsigned z=z1-1;z<=z2; z++)
                    {
                        if (m.array_of_rooms[x][y][z]->get_chamber_id()==0)
                            m.array_of_rooms[x][y][z]->assign_to_chamber(id);
                    }
                }
            }
            break;
    }
}



void skarabeusz::room::set_coordinates(unsigned nx, unsigned ny, unsigned nz)
{
    x = nx;
    y = ny;
    z = nz;
}

void skarabeusz::maze::create_chambers(unsigned aoc)
{
    amount_of_chambers = aoc;
    vector_of_chambers.resize(aoc);
    for (unsigned c=0; c<aoc; c++)
    {
        vector_of_chambers[c]=std::make_shared<chamber>();
        vector_of_chambers[c]->set_id(c+1);
    }
}


void skarabeusz::maze::resize(unsigned xr, unsigned yr, unsigned zr)
{
    max_x_range = xr;
    max_y_range = yr;
    max_z_range = zr;
    
    array_of_rooms.resize(xr);
    for (auto & x: array_of_rooms)
    {
        x.resize(yr);
        for (auto & y: x)
        {
            y.resize(zr);
        }
    }
    
    for (unsigned x=0; x<max_x_range; x++)
    {
        for (unsigned y=0; y<max_y_range; y++)
        {
            for (unsigned z=0; z<max_z_range; z++)
            {
                array_of_rooms[x][y][z]=std::make_shared<room>();
                array_of_rooms[x][y][z]->set_coordinates(x,y,z);
            }
        }
    }
}

void skarabeusz::maze::choose_seed_rooms(generator & g)
{    
    for (unsigned i=1; i<=amount_of_chambers; i++)
    {
        unsigned x,y,z;
        do
        {
            g.get_random_room(x, y, z);
        }
        while (array_of_rooms[x][y][z]->get_is_assigned_to_any_chamber());
        
        array_of_rooms[x][y][z]->assign_to_chamber(i, true);
        
        vector_of_chambers[i-1]->seed.x = x;
        vector_of_chambers[i-1]->seed.y = y;
        vector_of_chambers[i-1]->seed.z = z;
        
        DEBUG("seed room for " << i << " is " << x << "," << y << "," << z);
    }
}

void skarabeusz::maze::create_latex()
{
    std::ofstream file_stream("maze.tex");
    file_stream 
    << "\\documentclass[10pt,a4paper,titlepage,openbib]{book}\n"
    << "\\usepackage{ulem}\n"
    << "\\usepackage{geometry}\n"
    << "\\usepackage{graphicx}\n"
    << "\\usepackage{url}\n"
    << "\\usepackage{color}\n"
    << "\\usepackage{listings}\n"
    << "\\usepackage{tcolorbox}\n"
    << "\\usepackage[utf8]{inputenc}\n"
    << "\\usepackage[T1]{fontenc}\n"
    << "\\usepackage{hyperref}\n"
    << "\\hypersetup{\n"
	<< "   colorlinks=true,\n"
	<< "   linkcolor=blue,\n"
	<< "   filecolor=magenta,\n"
	<< "   urlcolor=cyan\n"
    << "}\n";
    
    
    file_stream
    << "\\begin{document}\n";
    
    for (unsigned z=0; z<max_z_range; z++)
    {        
        file_stream
        << "\\newpage\n"
        << "To jest mapa poziomu " << (z+1) << "-go:\\\\\n"
        << "\\includegraphics[width=16cm,height=12cm]{map_" << z << ".png}\\\\\n";
    }

    file_stream << "\\newpage\n";
    
    for (unsigned z=0; z<amount_of_paragraphs; z++)
    {
        vector_of_paragraphs[z]->print(file_stream);
    }
        
    file_stream
    << "\\end{document}\n";
}


void skarabeusz::hyperlink::print(std::ostream & s) const 
{ 
    s << "\\hyperlink{par " << my_paragraph.number << "}{" << my_paragraph.number << "}"; 
}

void skarabeusz::maze::create_html()
{
}

void skarabeusz::generator::get_random_room(unsigned & x, unsigned & y, unsigned & z)
{
    x = x_distr(gen);
    y = y_distr(gen);
    z = z_distr(gen);
}

void skarabeusz::maze::get_all_rooms_of_chamber(unsigned id, std::vector<std::shared_ptr<room>> & target) const
{
    for (unsigned x=0; x<max_x_range; x++)
    {
        for (unsigned y=0; y<max_y_range; y++)
        {
            for (unsigned z=0; z<max_z_range; z++)
            {
                if (array_of_rooms[x][y][z]->get_is_assigned_to_chamber(id))
                {
                    //std::cout << "room " << x << "," << y << "," << z << " is assigned to chamber " << id << "\n";
                    auto p{array_of_rooms[x][y][z]};
                    target.push_back(std::move(p));
                }
            }
        }
    }
}

unsigned skarabeusz::maze::get_x1_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->x < b->x; });
    
    return temporary_vector[0]->x;
}

unsigned skarabeusz::maze::get_y1_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->y < b->y; });
    
    return temporary_vector[0]->y;
}

unsigned skarabeusz::maze::get_z1_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->z < b->z; });
    
    return temporary_vector[0]->z;
}

unsigned skarabeusz::maze::get_x2_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->x > b->x; });
    
    return temporary_vector[0]->x;

}

unsigned skarabeusz::maze::get_y2_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->y > b->y; });
    
    return temporary_vector[0]->y;
}

unsigned skarabeusz::maze::get_z2_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->z > b->z; });
    
    return temporary_vector[0]->z;
}

bool skarabeusz::maze::get_can_be_assigned_to_chamber(unsigned x1, unsigned y1, unsigned z1, unsigned x2, unsigned y2, unsigned z2, unsigned id) const
{
    for (unsigned x=x1; x<=x2; x++)
    {
        for (unsigned y=y1; y<=y2; y++)
        {
            for (unsigned z=z1; z<=z2; z++)
            {
                if (!array_of_rooms[x][y][z]->get_can_be_assigned_to_chamber(id))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

void skarabeusz::maze::get_all_chambers_that_can_grow_horizontally(std::vector<std::shared_ptr<chamber>> & target) const
{
    for (auto & a: vector_of_chambers)
    {            
        if (a->get_can_grow_horizontally(*this))
        {
            auto b{a};
            target.push_back(std::move(b));
        }
    }
    
}


void skarabeusz::maze::get_all_chambers_that_can_grow_in_random_directions(std::vector<std::pair<std::shared_ptr<chamber>, room::direction_type>> & target) const
{
    for (auto & a: vector_of_chambers)
    {
        for (unsigned d=0; d<4; d++)    // only horizontally
        {
            room::direction_type direction=static_cast<room::direction_type>(d);
            
            if (a->get_can_grow_in_direction(direction, *this))
            {
                auto b{a};
                target.push_back(std::pair(std::move(b),direction));
            }
        }
    }
}


void skarabeusz::maze::get_all_chambers_that_can_grow_in_all_directions(std::vector<std::shared_ptr<chamber>> & target) const
{
    for (auto & a: vector_of_chambers)
    {
        if (a->get_can_grow_in_all_directions(*this))
        {
            auto b{a};
            target.push_back(std::move(b));
        }
    }
}


bool skarabeusz::chamber::get_can_grow_horizontally(const maze & m) const
{
    unsigned x1,y1,x2,y2,z1,z2;
    x1 = m.get_x1_of_chamber(id);
    y1 = m.get_y1_of_chamber(id);
    z1 = m.get_z1_of_chamber(id);
    x2 = m.get_x2_of_chamber(id);
    y2 = m.get_y2_of_chamber(id);
    z2 = m.get_z2_of_chamber(id);
    if (y1==0) return false;
    if (x2 == m.max_x_range-1) return false;
    if (y2==m.max_y_range-1) return false;
    if (x1 == 0) return false;
    return m.get_can_be_assigned_to_chamber(x1-1,y1-1,z1,x2+1,y2+1,z2, id);
}


bool skarabeusz::chamber::get_can_grow_in_direction(room::direction_type d, const maze & m) const
{
    unsigned x1,y1,x2,y2,z1,z2;
    x1 = m.get_x1_of_chamber(id);
    y1 = m.get_y1_of_chamber(id);
    z1 = m.get_z1_of_chamber(id);
    x2 = m.get_x2_of_chamber(id);
    y2 = m.get_y2_of_chamber(id);
    z2 = m.get_z2_of_chamber(id);

    switch (d)
    {
        case room::direction_type::NORTH:
            if (y1==0) return false;
            return m.get_can_be_assigned_to_chamber(x1,y1-1,z1,x2,y2,z2, id);
        
        case room::direction_type::EAST:
            if (x2 == m.max_x_range-1)
                return false;
            return m.get_can_be_assigned_to_chamber(x1,y1,z1,x2+1,y2,z2, id);
            
        case room::direction_type::SOUTH:
            if (y2==m.max_y_range-1) return false;
            return m.get_can_be_assigned_to_chamber(x1,y1,z1,x2,y2+1,z2, id);
            
        case room::direction_type::WEST:
            if (x1 == 0) return false;
            return m.get_can_be_assigned_to_chamber(x1-1,y1,z1,x2,y2,z2, id);
            
        case room::direction_type::UP:
            if (z2 == m.max_z_range-1) return false;
            return m.get_can_be_assigned_to_chamber(x1,y1,z1,x2,y2,z2+1, id);
            
        case room::direction_type::DOWN:
            if (z1 == 0) return false;
            return m.get_can_be_assigned_to_chamber(x1,y1,z1-1,x2,y2,z2, id);
            
    }
    return false;
}


bool skarabeusz::chamber::get_can_grow_in_all_directions(const maze & m) const
{
    unsigned x1,y1,x2,y2,z1,z2;
    x1 = m.get_x1_of_chamber(id);
    y1 = m.get_y1_of_chamber(id);
    z1 = m.get_z1_of_chamber(id);
    x2 = m.get_x2_of_chamber(id);
    y2 = m.get_y2_of_chamber(id);
    z2 = m.get_z2_of_chamber(id);
    
    //std::cout << "chamber " << id << " has dimensions " << x1 << ".." << x2 << "x" << y1 << ".." << y2 << "x" << z1 << ".." << z2 << "\n";
    
    if (x1 == 0)
        return false;
    if (x2 == m.max_x_range-1)
        return false;
    if (y1 == 0)
        return false;
    if (y2 == m.max_y_range-1)
        return false;
    if (z1 == 0)
        return false;
    if (z2 == m.max_z_range-1)
        return false;
    
    if (!m.get_can_be_assigned_to_chamber(x1-1,y1-1,z1-1,x2+1,y2+1,z2+1, id))
    {
        return false;
    }
    
    return true;
}
bool skarabeusz::room::get_is_connected(const maze & m, direction_type t) const
{
    switch (t)
    {
        case direction_type::NORTH:
            if (y==0) return false;
            if (m.get_room(x,y-1,z).get_chamber_id()==chamber_id)
                return true;
            break;
            
        case direction_type::EAST:
            if (x==m.max_x_range-1) return false;
            if (m.get_room(x+1,y,z).get_chamber_id()==chamber_id)
                return true;
            break;
            
        case direction_type::SOUTH:
            if (y==m.max_y_range-1) return false;
            if (m.get_room(x,y+1,z).get_chamber_id()==chamber_id)
                return true;
            break;
            
        case direction_type::WEST:
            if (x==0) return false;
            if (m.get_room(x-1,y,z).get_chamber_id()==chamber_id)
                return true;
            break;
        case direction_type::UP:
            if (z==m.max_z_range-1) return false;
            if (m.get_room(x,y,z+1).get_chamber_id()==chamber_id)
                return true;
            break;
        case direction_type::DOWN:
            if (z==0) return false;
            if (m.get_room(x,y,z-1).get_chamber_id()==chamber_id)
                return true;
            break;
    }
    return false;
}


void skarabeusz::maze::create_maps(const map_parameters & mp)
{
    for (unsigned z=0; z<max_z_range; z++)
    {
        image i(800, 600);
        
        int white = i.allocate_color(255, 255, 255);
        int black = i.allocate_color(0,0,0);
        int red = i.allocate_color(255,0,0);
        int blue = i.allocate_color(100,100,255);
        
        for (unsigned x=0; x<max_x_range; x++)
        {
            for (unsigned y=0; y<max_y_range; y++)
            {
                if (!array_of_rooms[x][y][z]->get_is_connected(*this, room::direction_type::NORTH))
                    i.line(x*mp.room_width, y*mp.room_height, 
                       x*mp.room_width+mp.room_width-1, y*mp.room_height,black);
                
                if (!array_of_rooms[x][y][z]->get_is_connected(*this, room::direction_type::WEST))
                    i.line(x*mp.room_width, y*mp.room_height,
                       x*mp.room_width, y*mp.room_height+mp.room_height-1, black);
                
                if (!array_of_rooms[x][y][z]->get_is_connected(*this, room::direction_type::SOUTH))
                    i.line(x*mp.room_width, y*mp.room_height+mp.room_height-1, 
                       x*mp.room_width+mp.room_width-1, y*mp.room_height+mp.room_height-1,black);
                    
                if (!array_of_rooms[x][y][z]->get_is_connected(*this, room::direction_type::EAST))
                    i.line(x*mp.room_width+mp.room_width-1, y*mp.room_height,
                       x*mp.room_width+mp.room_width-1, y*mp.room_height+mp.room_height-1, black);
                
                if (array_of_rooms[x][y][z]->get_is_seed_room())
                {
                    i.fill_ellipse(x*mp.room_width+mp.room_width/2,y*mp.room_height+mp.room_height/2,mp.room_width/2,mp.room_height/2, red);                    
                    i.print_string(x*mp.room_width+mp.room_width/2-10,y*mp.room_height+mp.room_height/2-10, array_of_rooms[x][y][z]->name, black);
                    
                }
                
                for (auto & d: array_of_rooms[x][y][z]->list_of_door)
                {
                    i.fill_ellipse((x+d->target_room.x)*mp.room_width/2+mp.room_width/2,(y+d->target_room.y)*mp.room_height/2+mp.room_height/2, mp.room_width/2,mp.room_height/2, blue);
                    i.print_string(x*mp.room_width+mp.room_width/2-10,y*mp.room_height+mp.room_height/2-10, array_of_rooms[x][y][z]->name, black);
                }
                
            }
            
        }

        
        
        
        FILE * pngout=nullptr;
        
        std::stringstream map_name_stream;
        
        map_name_stream << "map_" << z << ".png";
        
        pngout = fopen(map_name_stream.str().c_str(), "wb");
        gdImagePng(i.get_image_pointer(), pngout);
        fclose(pngout);
    }
}

int main(int argc, char * argv[])
{
    unsigned x_range=15,y_range=10,z_range=1,amount_of_chambers=5,max_amount_of_keys_to_hold=2,amount_of_alternative_endings=1;
    
    for (unsigned i=1; i<argc; i++)
    {
        if (!strcmp(argv[i], "-x"))
        {
            x_range = atoi(argv[++i]);
        }
        else
        if (!strcmp(argv[i], "-y"))
        {
            y_range = atoi(argv[++i]);
        }
        else
        if (!strcmp(argv[i], "-z"))
        {
            z_range = atoi(argv[++i]);
        }
        else
        if (!strcmp(argv[i], "-c"))
        {
            amount_of_chambers = atoi(argv[++i]);
        }
        else
        if (!strcmp(argv[i], "-a"))
        {
            amount_of_alternative_endings = atoi(argv[++i]);
        }        
        else
        {
            std::cerr << "unrecognized option " << argv[i] << "\n";
            exit(1);
        }
    }
    
    skarabeusz::generator_parameters gp{x_range,y_range,z_range, // x,y,z
                                        3, // max amount of magic items 
                                        amount_of_chambers, // amount of chambers
                                        max_amount_of_keys_to_hold,// max amount of keys to hold
                                        amount_of_alternative_endings};
    skarabeusz::maze m;
    
    skarabeusz::generator g{gp, m};
    
    g.run();    
    
    skarabeusz::map_parameters mp{80,80};
    
    m.create_maps(mp);
    m.create_latex();
    
    return 0;
}
