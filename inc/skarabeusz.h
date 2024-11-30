#ifndef SKARABEUSZ_H
#define SKARABEUSZ_H
#include <vector>
#include <iostream>
#include <random>
#include <memory>
#include <list>
#include <map>
#include <limits>
#include <gd.h>
#include <gdfontg.h>

namespace skarabeusz
{
    class map_parameters
    {
    private:
        friend class maze;
        unsigned room_width, room_height;
    public:
        map_parameters(unsigned rw, unsigned rh): room_width{rw}, room_height{rh} {}
    };
    
    class generator_parameters
    {
    private:
        friend class generator;
        
        unsigned maze_x_range, maze_y_range, maze_z_range;
        
        unsigned max_amount_of_keys_to_hold;
        
        unsigned max_amount_of_magic_items;
        
        unsigned amount_of_chambers;
        
        unsigned amount_of_alternative_endings;
                
        bool hints;
    public:
        enum class output_mode_type { NONE=0x0, LATEX=0x1, HTML=0x2 };
        
        enum class html_generating_mode { NONE=0x0, SINGLE_PAGE=0x1, MULTIPLE_PAGES = 0x2 };
    
    private:
        output_mode_type output_mode;
    public:
        generator_parameters(unsigned xr, unsigned yr, unsigned zr, unsigned maomi, unsigned aoc, unsigned maokth, unsigned aoae, bool h):
            maze_x_range(xr), maze_y_range(yr), maze_z_range(zr),
            max_amount_of_magic_items{maomi},
            amount_of_chambers{aoc},
            max_amount_of_keys_to_hold{maokth},            
            output_mode{output_mode_type::LATEX},
            amount_of_alternative_endings{aoae},
            hints{h}
            {
            }
    };
    
    class image
    {
    private:
        gdImagePtr i;
    public:
        image(unsigned width, unsigned height)
        {
            i = gdImageCreateTrueColor(width, height);
            int white = gdImageColorAllocate (i, 255, 255, 255);
            gdImageFilledRectangle (i, 0, 0, gdImageSX (i), gdImageSY (i), white);
        }
        image(const std::string & filename)
        {
            i = gdImageCreateFromFile(filename.c_str());
            if (!i)
            {
                throw std::runtime_error("could not open file");
            }
        }        
        
        gdImagePtr get_image_pointer() { return i; }
        
        int allocate_color(unsigned r, unsigned g, unsigned b)
        {
            return gdImageColorAllocate(i, r, g, b);
        }
        
        void line(unsigned x1, unsigned y1, unsigned x2, unsigned y2, int c)
        {
            gdImageLine(i, x1, y1, x2, y2, c);
        }
        
        void fill_ellipse(unsigned x, unsigned y, unsigned w, unsigned h, int c)
        {
            gdImageFilledEllipse(i, x, y, w, h, c);
        }
        
        void print_string(int x, int y, const std::string & s, int c)
        {
            gdImageString(i, gdFontGetGiant(), x, y, (unsigned char*)s.c_str(), c);
        }
        
        void copy(const image & other, int dstX, int dstY, int width, int height)
        {
            gdImageCopy(i, other.i, dstX, dstY, 0, 0, width, height);
        }
        
        
        virtual ~image()
        {
            gdImageDestroy(i);
        }
    };
        
    class room;
    class maze;
    
    class virtual_door
    {
    private:
        std::list<std::vector<std::vector<bool>>> list_of_matrices;
        const unsigned chamber_id;
    public:
        virtual_door(unsigned i): chamber_id{i} {}
		void push_back(const std::vector<std::vector<bool>> & s) { list_of_matrices.push_back(s); }
		bool get_can_exchange(const std::vector<std::vector<bool> > & k1, const std::vector<std::vector<bool> > & k2) const;
        bool get_contains(const std::vector<std::vector<bool> > & k) const;
        void add_keys_from(const virtual_door & d);
        
        std::string get_description(const maze & m, std::list<std::vector<std::vector<bool>>>::const_iterator x) const;
        
        unsigned get_chamber_id() const { return chamber_id; }
        std::list<std::vector<std::vector<bool>>> & get_list_of_matrices() { return list_of_matrices; }
    };
    
    class maze;
    
    class collection_of_virtual_door
	{
	private:
		std::list<virtual_door> list_of_virtual_door;        
    public:
		void push_back(const virtual_door & d) { list_of_virtual_door.push_back(d); }  
		
		bool get_can_exchange(const std::vector<std::vector<bool>> & k1, const std::vector<std::vector<bool>> & k2) const;
        
        void merge_classes(const std::vector<std::vector<bool>> & k1, const std::vector<std::vector<bool>> & k2);
        
        std::string get_description(const maze & m) const;
        
        std::list<virtual_door> & get_list_of_virtual_door() { return list_of_virtual_door; }
    };

    
    class door;
    class keys;
    class door_object;
    
    class room
    {
    public:
        enum class direction_type { NORTH, EAST, SOUTH, WEST, UP, DOWN };
    private:        
        unsigned x,y,z;
        
        unsigned chamber_id;
        
        bool is_seed_room;
        
        std::string name;
        
        std::list<std::shared_ptr<door>> list_of_door;                
        
    public:
        room(): chamber_id{0}, x{0} , y{0}, z{0}, is_seed_room{false} {}
        
        void set_coordinates(unsigned nx, unsigned ny, unsigned nz);
        
        bool get_is_assigned_to_any_chamber() const { return chamber_id > 0; }
        
        bool get_is_seed_room() const { return is_seed_room; }
        
        void assign_to_chamber(unsigned ci, bool s=false) 
        { chamber_id = ci; is_seed_room=s; }
        
        bool get_is_assigned_to_chamber(unsigned ci) const { return chamber_id == ci; }
        
        bool get_can_be_assigned_to_chamber(unsigned ci) { return chamber_id == 0 || chamber_id == ci; }
        
        bool get_is_connected(const maze & m, direction_type t) const;
        
        unsigned get_chamber_id() const { return chamber_id; }
        
        const std::list<std::shared_ptr<door>>& get_list_of_door() const { return list_of_door; }

        bool get_has_door_object(door_object & d) const;
        
        void set_name(const std::string & n);
        
        bool get_has_door_leading(direction_type d) const;
        
        bool get_door_can_be_opened_with(direction_type d, const keys & k) const;
        
        std::string which_key_can_open(direction_type d) const;
        
        unsigned get_x() const { return x; }
        unsigned get_y() const { return y; }
        unsigned get_z() const { return z; }
        const std::string get_name() const { return name; }
        std::list<std::shared_ptr<door>> & get_list_of_door() { return list_of_door; }
    };
    
    class door_object
    {
    private:
        unsigned chamber1,chamber2;
        unsigned key_number;    // begins with 1
        const std::string key_name, with_the_key_name;
    public:
        door_object(unsigned c1, unsigned c2, unsigned kn, const std::string & k, const std::string & wtk): 
            chamber1{c1}, chamber2{c2}, key_number{kn}, 
            key_name{k}, with_the_key_name{wtk} {}
        
        bool get_connects(unsigned c1, unsigned c2) const { return (chamber1==c1 && chamber2==c2) || (chamber1==c2 && chamber2==c1); }
        
        const std::string & get_key_name() const { return key_name; }
        
        const std::string & get_with_the_key_name() const { return with_the_key_name; }

        bool operator==(const door_object & d) const
        {
            return chamber1 == d.chamber1 && chamber2 == d.chamber2 && key_number == d.key_number;
        }
    };
    

    class door
    {
    private:                
        room & target_room;
        unsigned chamber1,chamber2;
        room::direction_type direction;
        
        door_object & my_object;
        
        friend class generator;
    public:
        door(unsigned c1, unsigned c2, room & r, room::direction_type d, door_object & o): 
            chamber1{c1}, chamber2{c2}, target_room{r}, direction{d}, my_object{o} {}
        
        room::direction_type get_direction() const { return direction; }
        
        room & get_target_room() { return target_room; }
        
        unsigned get_chamber1() const { return chamber1; }
        unsigned get_chamber2() const { return chamber2; }
        
        door_object & get_door_object() { return my_object; }
    };
    
    class chamber
    {
    private:
        unsigned id;        
        struct seed_type 
        { 
            unsigned x,y,z; 
            void set_x(unsigned nx) { x=nx; }
            void set_y(unsigned ny) { y=ny; }
            void set_z(unsigned nz) { z=nz; }
        } seed;
        std::string guardian_name;
    public:
        chamber(): id{0} {}
        
        void set_guardian_name(const std::string & gn) { guardian_name = gn; }
        
        std::string get_guardian_name() const { return guardian_name; }
        
        void set_id(unsigned nid) { id = nid; }
        
        bool get_can_grow_in_all_directions(const maze & m) const;
        
        bool get_can_grow_in_direction(room::direction_type d, const maze & m) const;
        
        bool get_can_grow_horizontally(const maze & m) const;
        
        void grow_in_all_directions(maze & m) const;
        
        void grow_in_direction(room::direction_type d, maze & m) const;
        
        std::string get_description(const maze & m) const;
        
        const seed_type & get_seed() const { return seed; }
        seed_type & get_seed() { return seed; }
    };
    
    
    class generator;
    
    class keys
    {
    private:        
        std::vector<std::vector<bool>> matrix;
    public:
        keys(const maze & m);
        
        keys(const std::vector<std::vector<bool>> & m): matrix{m} {}
        
        unsigned get_amount_of_keys() const;
        
        bool get_can_exit_chamber(unsigned chamber_id) const;
        
        bool operator==(const keys & other) const;
        
        bool operator!=(const keys & other) const { return !this->operator==(other); }
        
        keys& operator=(bool v);
        
        void report(std::ostream & s) const;
        
        std::vector<std::vector<bool>> & get_matrix() { return matrix; }
        
        const std::vector<std::vector<bool>> & get_matrix() const { return matrix; }
    };
    
    /**
     * This class represents a unique player situation, i.e. where he is located
     * and what keys he has.
     */
    class chamber_and_keys
    {
    private:
        unsigned chamber_id;
        keys my_keys;
        bool done;
    public:
        chamber_and_keys(unsigned c, const std::vector<std::vector<bool>> & m): chamber_id{c}, my_keys{m}, done{false} {}
        chamber_and_keys(unsigned c, const keys & k): chamber_id{c}, my_keys{k}, done{false} {}
        
        bool get_done() const { return done; }
        
        void set_done(bool d) { done = d; }
        
        keys& get_keys() { return my_keys; }

        const keys& get_keys() const { return my_keys; }
        
        unsigned get_chamber_id() const { return chamber_id; }

        bool operator==(const chamber_and_keys & c) const { return chamber_id == c.chamber_id && my_keys == c.my_keys; }

        bool operator!=(const chamber_and_keys & c) const { return chamber_id != c.chamber_id || my_keys != c.my_keys; }

    };
    
    class paragraph_item
    {
    public:
        virtual ~paragraph_item() {}
        
        virtual void print_latex(std::ostream & s) const = 0;
        virtual void print_html(std::ostream & s) const = 0;
        virtual void print_text(std::ostream & s) const = 0;

    };
    
    
    class normal_text: public paragraph_item
    {
    private:
        const std::string text;
    public:
        normal_text(const std::string & t): text{t} {}
        virtual void print_latex(std::ostream & s) const override { s << text; }
        virtual void print_html(std::ostream & s) const override { s << text; }
        virtual void print_text(std::ostream & s) const override { s << text; }
    };
    
    class paragraph;
    
    class hyperlink: public paragraph_item
    {
    private:
        const paragraph & my_paragraph;
    public:
        hyperlink(const paragraph & p): my_paragraph{p} {}
        virtual void print_latex(std::ostream & s) const override;
        virtual void print_html(std::ostream & s) const override;
        virtual void print_text(std::ostream & s) const override;
    };

    class hint_you_still_need_i_steps: public paragraph_item
    {
    private:
        const paragraph & my_paragraph;
    public:
        hint_you_still_need_i_steps(const paragraph & p): my_paragraph{p} {}
        virtual void print_latex(std::ostream & s) const override;
        virtual void print_html(std::ostream & s) const override;
        virtual void print_text(std::ostream & s) const override;
    };
    

    class paragraph_connection
    {
    friend class generator;
    private:
        paragraph &first, &second;
        bool has_been_processed;
    public:
        paragraph_connection(paragraph & f, paragraph & s):
            first{f}, second{s} {}

        void set_has_been_processed(bool p)
        {
            has_been_processed = p;
        }

        bool get_has_been_processed() const { return has_been_processed; }

        const paragraph & get_first() const { return first; }
        const paragraph & get_second() const { return second; }
    };
    
    class paragraph
    {
    friend class generator;
    friend class hint_you_still_need_i_steps;
    public:
        enum class paragraph_type { CHAMBER_DESCRIPTION, DISCUSSION, DOOR };
    private:
        bool ending;
        int distance_to_the_closest_ending; // we may have many endings!
        bool distance_has_been_calculated;

        unsigned number;        
        chamber_and_keys my_state;
        std::list<std::unique_ptr<paragraph_item>> list_of_paragraph_items;
        paragraph_type type;
        unsigned x,y,z;
    public:
        paragraph(unsigned n, const chamber_and_keys & c, unsigned nx, unsigned ny, unsigned nz, paragraph_type t);

        bool get_distance_has_been_calculated() const { return distance_has_been_calculated; }

        int get_distance_to_the_closest_ending() const { return distance_to_the_closest_ending; }

        void set_distance_to_the_closest_ending(int d)
        {
            distance_to_the_closest_ending = d;
            distance_has_been_calculated = true;
        }

        void add(std::unique_ptr<paragraph_item> && i) { list_of_paragraph_items.push_back(std::move(i)); }
        
        void print_latex(std::ostream & s) const;        
        void print_html(std::ostream & s) const;
        void print_text(std::ostream & s) const;
        
        chamber_and_keys& get_state() { return my_state; }
        
        bool get_ending() const { return ending; }
        
        void set_ending(bool e) { ending = e; }
        
        void set_number(unsigned n) { number = n; }
        
        unsigned get_number() const { return number; }
        
        unsigned get_x() const { return x; }
        unsigned get_y() const { return y; }
        unsigned get_z() const { return z; }
        
        paragraph_type get_type() const { return type; }
    };

    class resource
    {
    private:
        const std::string name;
    public:
        resource(const std::string & n): name{n} {}
        std::string get_name() const { return name; }
        virtual ~resource() {}
    };
    
    class resource_image: public resource
    {
    private:
        image my_image;
    public:
        resource_image(const std::string & n, const std::string & filename): resource{n}, my_image{filename} {}
        
        const image& get_image() const { return my_image; }
    };
    
    class resources
    {
    private:
        std::vector<std::unique_ptr<resource>> vector_of_resources;
        
        std::map<std::string, int> map_resource_name_to_index;
        
    public:
        void add_resource(std::unique_ptr<resource> && r) 
        { 
            map_resource_name_to_index.insert(std::pair(r->get_name(), vector_of_resources.size()));
            vector_of_resources.push_back(std::move(r)); 
        }
        
        const resource_image& get_resource_image(const std::string & n) const
        {
            if (map_resource_name_to_index.find(n) == map_resource_name_to_index.end())
            {
                throw std::runtime_error("could not find the resource");
            }
            
            const resource_image& ri{dynamic_cast<resource_image&>(*vector_of_resources[map_resource_name_to_index.at(n)])};
            return ri;
        }
    };
    
    class maze
    {
    private:
        friend class chamber;
        friend class room;
        friend class keys;
        friend class generator;
        friend class collection_of_virtual_door;
        unsigned amount_of_chambers, amount_of_keys, amount_of_paragraphs;
        
        unsigned max_x_range, max_y_range, max_z_range;
        
        std::vector<std::vector<std::vector<std::shared_ptr<room>>>> array_of_rooms;
        
        std::vector<std::shared_ptr<chamber>> vector_of_chambers;
        
        /**
         * For chamber1 and chamber2 this matrix returns the information whether one can go directly from chamber1 to chamber2 - 
         * it is matrix_of_chamber_transitions[chamber1-1][chamber2-1]
         */
        std::vector<std::vector<bool>> matrix_of_chamber_transitions;

		std::vector<collection_of_virtual_door> vector_of_virtual_door;

        std::vector<std::unique_ptr<paragraph>> vector_of_paragraphs;
        
        std::vector<std::unique_ptr<door_object>> vector_of_door_objects;

        std::vector<std::unique_ptr<paragraph_connection>> vector_of_paragraph_connections;
        
        
    public:
        maze(): amount_of_keys{0} {}
        
        void get_all_rooms_of_chamber(unsigned id, std::vector<std::shared_ptr<room>> & target) const;
        
        std::vector<std::vector<bool>> & get_keys()
        {
            return matrix_of_chamber_transitions;
        }
        
        const room& get_room(unsigned x, unsigned y, unsigned z) const
        {
            return *array_of_rooms[x][y][z];
        }
        
        void get_all_chambers_that_can_grow_in_all_directions(std::vector<std::shared_ptr<chamber>> & target) const;
        
        void get_all_chambers_that_can_grow_horizontally(std::vector<std::shared_ptr<chamber>> & target) const;
        
        void get_all_chambers_that_can_grow_in_random_directions(std::vector<std::pair<std::shared_ptr<chamber>, room::direction_type>> & target) const;
        
        void resize(unsigned xr, unsigned yr, unsigned zr);
        
        void create_chambers(unsigned aoc);
                
        void create_html(const std::string & prefix, const std::string & html_head_file);        
        void create_html_index(const std::string & prefix, const std::string & html_head);        
        void create_latex(const std::string & prefix);
        
        void create_maps_latex(const map_parameters & mp, const std::string & prefix);
        void create_maps_html(const map_parameters & mp, const std::string & prefix, const resources & r);
        
        void choose_seed_rooms(generator & g);
        
        unsigned get_x1_of_chamber(unsigned id) const;
        unsigned get_y1_of_chamber(unsigned id) const;
        unsigned get_z1_of_chamber(unsigned id) const;
        unsigned get_x2_of_chamber(unsigned id) const;
        unsigned get_y2_of_chamber(unsigned id) const;
        unsigned get_z2_of_chamber(unsigned id) const;
        
        bool get_can_be_assigned_to_chamber(unsigned x1, unsigned y1, unsigned z1, unsigned x2, unsigned y2, unsigned z2, unsigned id) const;

        bool get_all_paragraph_connections_have_been_processed() const;
        
        void grow_chambers_in_all_directions(generator & g);
        
        void grow_chambers_horizontally(generator & g);
        
        void grow_chambers_in_random_directions(generator & g);
        
        void generate_room_names();
        
        void generate_door(generator & g);
        
        std::string get_keys_description(const keys & k) const;
        
        std::string get_keys_description_after_exchange(const keys & k) const;
        
        std::string get_wall_description(unsigned chamber_id, room::direction_type d) const;
        
        unsigned get_paragraph_index(unsigned x, unsigned y, unsigned z, const keys & k, paragraph::paragraph_type t) const;
        
        unsigned get_paragraph_index(unsigned chamber_id, const keys & k, paragraph::paragraph_type t) const;
        
        door_object & get_door_object(unsigned chamber1, unsigned chamber2);
        
        const door_object & get_door_object(unsigned chamber1, unsigned chamber2) const;

        paragraph& get_discussion_paragraph(chamber_and_keys & c);

        paragraph& get_description_paragraph(chamber_and_keys & c);

        paragraph& get_door_paragraph(chamber_and_keys & c, int index);

        paragraph& get_door_paragraph(chamber_and_keys & c, door_object & d);

        int get_amount_of_door_in_chamber(chamber_and_keys & c);

        void report(std::ostream & s) const;
    };

    
    class generator
    {
    private:
        const generator_parameters & parameters;
        maze & target;
        
        std::random_device rd;
        std::mt19937 gen;
        
        std::uniform_int_distribution<unsigned> x_distr, y_distr, z_distr;
        
        std::list<std::pair<unsigned, keys>> list_of_pairs_chamber_and_keys, list_of_processed_pairs_chamber_and_keys;
        std::list<keys> list_of_keys;
        std::vector<chamber_and_keys> vector_of_pairs_chamber_and_keys;


        /**
         * This vector of lists contains the equivalence classes corresponding with the chambers.
         * For each chamber there is a number of lists of matrices of chamber transitions corresponding with it.
         * Each matrix represents the combination of keys one can get in this chamber.
		*/
		std::vector<std::list<std::list<keys>>> vector_of_equivalence_classes;

        static const std::string names[];
        static const std::string key_names[];
        static const std::string with_the_x_key_names[];

        std::map<std::string, bool> map_name_to_taken_flag;
        
        void generate_list_of_keys();
        void generate_list_of_pairs_chamber_and_keys();
        void distribute_the_pairs_into_equivalence_classes();
        void add_pair_to_the_equivalence_classes(const std::pair<unsigned, keys> & p);        
        std::list<std::pair<unsigned, keys>>::iterator get_next_pair_to_connect(bool & no_need_to_connect);
        std::list<std::pair<unsigned, keys>>::iterator get_next_pair_to_connect_1();
        std::list<std::pair<unsigned, keys>>::iterator get_next_pair_to_connect_2();
        void add_and_connect_pair_to_the_equivalence_classes(const std::pair<unsigned, keys> & p, bool no_need_to_connect);
        void copy_the_equivalence_classes_to_the_maze();
        void process_the_equivalence_classes();
        void process_a_journey(unsigned chamber1, keys & keys1, unsigned chamber2, keys & keys2);
        bool process_a_journey_step();
        bool get_there_is_a_transition(const chamber_and_keys & p1, const chamber_and_keys & p2);
        bool get_all_pairs_have_been_done() const;
        void process_a_journey_get_candidate(unsigned & candidate_done, unsigned & candidate_not_done) const;
        void generate_paragraphs();
        void calculate_min_distance_to_the_closest_ending();
        void generate_names();        
        std::string get_random_name();
        unsigned get_amount_of_pairs_that_have_been_done() const;
        
    public:
        generator(const generator_parameters & gp, maze & t): 
            parameters{gp}, target{t}, gen{rd()},
            x_distr{0, parameters.maze_x_range-1},
            y_distr{0, parameters.maze_y_range-1},
            z_distr{0, parameters.maze_z_range-1} {}
        
        void run();
        
        void get_random_room(unsigned & x, unsigned & y, unsigned & z);
        void get_random_room_on_level_z(unsigned & x, unsigned & y, unsigned z);
        void get_random_key_name(std::string & normal_form, std::string & with_the_key_form);
        
        std::mt19937 & get_random_number_generator() { return gen; }
                
    };
}

std::ostream& operator<<(std::ostream & s, const skarabeusz::paragraph_connection & pc);
std::ostream& operator<<(std::ostream & s, const skarabeusz::paragraph & p);

#endif

