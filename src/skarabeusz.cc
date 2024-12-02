#include "skarabeusz.h" 
#include "config.h"
#include <libintl.h>
#include <locale.h>
#include <cstring>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <fstream>

#define _(STRING) gettext(STRING)
#define gettext_noop(STRING) STRING

#define SKARABEUSZ_MAX_MESSAGE_BUFFER 1000

//#define SKARABEUSZ_DEBUG

#ifdef SKARABEUSZ_DEBUG
#define DEBUG(X) std::cout << __LINE__ << ":" << X << "\n"
#else
#define DEBUG(X)
#endif


std::ostream& operator<<(std::ostream & s, const skarabeusz::paragraph_connection & pc)
{
    s << pc.get_first().get_number() << "->" << pc.get_second().get_number();
    return s;
}

std::ostream& operator<<(std::ostream & s, const skarabeusz::paragraph & p)
{
    p.print_text(s);
    return s;
}


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
    generate_chambers_guardians();
    generate_chambers_names();

    generate_heroes(); // required for stories and books
    generate_paragraphs();
    process_door_paragraphs_connections(),
    calculate_min_distance_to_the_closest_ending();

    if (parameters.bootstrap)
    {
        set_bootstrap_mode();
        target.bootstrap = true;
    }

    if (parameters.stories)
    {
        generate_stories();
        generate_stories_interpretations();
    }
}

const std::regex skarabeusz::character::regex_to_extract_family_name("\\w+ (\\w+)");

const std::string skarabeusz::character::get_family_name() const
{
    std::smatch result;
    if (std::regex_match(my_name, result, regex_to_extract_family_name))
    {
        return result[1].str();
    }

    throw std::runtime_error("internal error - could not match a name");

    return "???"; // should never happen
}

void skarabeusz::generator::set_bootstrap_mode()
{
    for (auto & p:target.vector_of_paragraphs)
    {
        p->bootstrap = true;
    }
}

void skarabeusz::generator::process_door_paragraphs_connections()
{
    for (auto & pc:target.vector_of_paragraph_connections)
    {
        if (pc->first.get_type()==paragraph::paragraph_type::DOOR)
        {
            pc->first.add_door_object_index(pc->door_object_index);
        }
        if (pc->second.get_type()==paragraph::paragraph_type::DOOR)
        {
            pc->second.add_door_object_index(pc->door_object_index);
        }
    }

    int amount_of_inconsistencies = 0;
    for (auto & p:target.vector_of_paragraphs)
    {
        if (p->get_type() == paragraph::paragraph_type::DOOR && p->vector_of_door_object_indeces.size() == 0)
        {
            amount_of_inconsistencies++;
        }
    }

    if (amount_of_inconsistencies > 0)
    {
        std::cerr << "there are " << amount_of_inconsistencies << " paragraphs with door but no door object!\n";
        parameters.stories = false;
        //throw std::runtime_error("inconsistency");
    }
}

void skarabeusz::generator::generate_chambers_names()
{
    for (auto & c: target.vector_of_chambers)
    {
        c->set_name(c->my_guardian->get_family_name());
    }
}


void skarabeusz::generator::generate_stories_interpretations()
{
    for (auto & p: target.vector_of_paragraphs)
    {
        p->generate_stories_interpretations(*this);
    }
}


void skarabeusz::paragraph::generate_stories_interpretations(generator & g)
{
    for (auto & pi: list_of_paragraph_items)
    {
        pi->generate_stories_interpretations(g);
    }
}

int skarabeusz::generator::get_random_story_index()
{
    assert(target.vector_of_stories.size() >=1);
    std::uniform_int_distribution<int> distr(0, target.vector_of_stories.size()-1);
    int index = distr(get_random_number_generator());
    return index;
}

int skarabeusz::generator::get_random_fact_index_from_the_story(int story_index, skarabeusz::character::alignment_type a)
{
    int index = 0;
    switch (a)
    {
        case character::alignment_type::GOOD:
        {
            std::uniform_int_distribution<int> distr(0, target.vector_of_stories[story_index]->vector_of_facts.size()-1);
            index = distr(get_random_number_generator());

            if (index > 0) index--;
            if (index > 0) index--;
        }
        break;
        case character::alignment_type::BAD:
        {
            std::uniform_int_distribution<int> distr(0, target.vector_of_stories[story_index]->vector_of_facts.size()-1);
            index = distr(get_random_number_generator());
            if (index < target.vector_of_stories[story_index]->vector_of_facts.size()-1) index++;
            if (index < target.vector_of_stories[story_index]->vector_of_facts.size()-1) index++;
        }
        break;
    }
    return index;
}


skarabeusz::story_interpretation::fact_interpretation::fact_interpretation(maze & m,
                    story::fact & f,
                    const character::alignment_type teller_alignment,
                    bool teller_likes_author, bool teller_likes_hero, paragraph & here_and_now):
                    my_maze{m},
                    my_fact{f},
                    my_teller_alignment{teller_alignment},
                    my_teller_likes_author{teller_likes_author},
                    my_teller_likes_hero{teller_likes_hero},
                    my_here_and_now{here_and_now}
{
}

void skarabeusz::story_interpretation::generate_stories_interpretations(generator & g)
{
    my_story_index = g.get_random_story_index(); // choose the story

    /*
    std::cout << "the amount of all stories " << g.target.vector_of_stories.size() << "\n";
    std::cout << "the interpreted story " << my_story_index << "\n";
    */

    does_the_teller_like_the_author = teller_alignment == g.target.vector_of_stories[my_story_index]->story_author.my_alignment;

    does_the_teller_like_the_hero = teller_alignment == g.target.vector_of_stories[my_story_index]->story_hero.my_alignment;

    // we are going to select some facts of the story
    // and create their interpretation
    /*
    std::cout << "interpreting the story " << my_story_index << "\n";
    std::cout << "does the teller like the author? " << does_the_teller_like_the_author << "\n";
    std::cout << "does the teller like the hero? " << does_the_teller_like_the_hero << "\n";
    */
    //std::cout << "the story contains " << g.target.vector_of_stories[my_story_index]->vector_of_facts.size() << " facts\n";

    int pivot_index = 0;

    switch (teller_alignment)
    {
        case character::alignment_type::GOOD:
        case character::alignment_type::BAD:
            for (int i = 0; i<g.target.vector_of_stories[my_story_index]->vector_of_facts.size(); i++)
            {
                vector_of_facts_interpretations.push_back(std::make_unique<fact_interpretation>(
                    my_maze, *g.target.vector_of_stories[my_story_index]->vector_of_facts[i],
                    teller_alignment,
                    does_the_teller_like_the_author, does_the_teller_like_the_hero, where_the_story_is_told));
            }
            break;
    }

    //std::cout << "\n";
}


// all authors are the guardians of the chambers
int skarabeusz::generator::get_random_author_index()
{
    std::uniform_int_distribution<int> distr(0, target.vector_of_chambers.size()-1);
    int index = distr(get_random_number_generator());
    return index;
}

int skarabeusz::generator::get_random_hero_index()
{
    std::uniform_int_distribution<int> distr(0, target.vector_of_heroes.size()-1);
    int index = distr(get_random_number_generator());
    return index;
}

int skarabeusz::generator::get_random_paragraph_index()
{
    std::uniform_int_distribution<int> distr(0, target.vector_of_paragraphs.size()-1);
    int index = distr(get_random_number_generator());
    return index;
}


bool skarabeusz::generator::get_there_are_paragraphs_leading_to(unsigned paragraph_index) const
{
    return std::find_if(target.vector_of_paragraphs.begin(), target.vector_of_paragraphs.end(), [paragraph_index, this](auto & p){ return get_leads_to_paragraph(*p, paragraph_index); }) != target.vector_of_paragraphs.end();
}

bool skarabeusz::generator::get_there_is_a_paragraph_connection(const paragraph & p1, const paragraph & p2) const
{
    return std::find_if(target.vector_of_paragraph_connections.begin(),
                        target.vector_of_paragraph_connections.end(),[&p1,&p2,this](auto & pc)
                        { return &pc->first == &p1 && &pc->second == &p2
                                || &pc->first == &p2 && &pc->second == &p1; })!=target.vector_of_paragraph_connections.end();
}

bool skarabeusz::generator::get_leads_to_paragraph(const paragraph & p, int paragraph_index) const
{
    return target.vector_of_paragraphs[paragraph_index]->distance_to_the_closest_ending < p.distance_to_the_closest_ending && get_there_is_a_paragraph_connection(p, *target.vector_of_paragraphs[paragraph_index ]);
}


int skarabeusz::generator::get_random_paragraph_index_leading_to(int paragraph_index)
{
    std::vector<int> potential_results;
    for (int i=0; i<target.vector_of_paragraphs.size(); i++)
    {
        if (get_leads_to_paragraph(*target.vector_of_paragraphs[i], paragraph_index))
        {
            potential_results.push_back(i);
        }
    }
    std::uniform_int_distribution<unsigned> distr(0, potential_results.size());
    int index = distr(get_random_number_generator());
    return index;
}


skarabeusz::character::alignment_type skarabeusz::generator::get_random_story_alignment(character::alignment_type author_alignment)
{
    switch (parameters.how_to_generate_stories)
    {
        case generator_parameters::stories_mode_type::GOOD: return character::alignment_type::GOOD;
        case generator_parameters::stories_mode_type::BAD: return character::alignment_type::BAD;
        case generator_parameters::stories_mode_type::RANDOM:
        {
            std::uniform_int_distribution<unsigned> distr(0, 1);
            int choice = distr(get_random_number_generator());
            switch (choice)
            {
                case 0: return character::alignment_type::BAD;
                case 1: return character::alignment_type::GOOD;
            }
        }
        break;

        case generator_parameters::stories_mode_type::ALIGNMENT_SPECIFIC: return author_alignment;
    }

    return character::alignment_type::GOOD;
}


void skarabeusz::generator::generate_stories()
{
    for (int i=0; i<parameters.amount_of_heroes; i++)
    {
        // for each hero we generate 3 stories
        // that will be later interpreted by the tellers
        character & my_story_hero = *target.vector_of_heroes[i];
        character & my_story_author = *target.vector_of_chambers[get_random_author_index()]->my_guardian;

        for (int j=0; j<amount_of_stories_per_hero; j++)
        {
            int attempts=0;
            do
            {
                auto s = std::make_unique<story>(my_story_author, my_story_hero, get_random_story_alignment(my_story_author.my_alignment));


                unsigned last_paragraph_index = get_random_paragraph_index();
                auto last_fact = std::make_unique<story::fact>(*target.vector_of_paragraphs[last_paragraph_index]);

                s->vector_of_facts.push_back(std::move(last_fact));

                for (int k=0; k<amount_of_facts_per_story; k++)
                {
                    if (!get_there_are_paragraphs_leading_to(last_paragraph_index))
                    {
                        break;
                    }

                    unsigned earlier_index = get_random_paragraph_index_leading_to(last_paragraph_index);

                    s->vector_of_facts.push_back(std::make_unique<story::fact>(*target.vector_of_paragraphs[earlier_index]));

                    last_paragraph_index = earlier_index;
                }

                switch (s->story_alignment)
                {
                    case character::alignment_type::BAD:
                        std::reverse(s->vector_of_facts.begin(), s->vector_of_facts.end());
                        break;

                    case character::alignment_type::GOOD:
                        // leave it as it is
                        break;
                }

                if (s->get_amount_of_facts() > 1)
                {
                    target.vector_of_stories.push_back(std::move(s));
                    break;
                }

                attempts++;

                if (attempts>1000)
                {
                    std::cerr << "The maze structure makes it impossible to create long stories\n";
                    exit(1);
                }
            }
            while (true);
        }
    }
}


void skarabeusz::generator::generate_heroes()
{
    for (int i=0; i<parameters.amount_of_heroes; i++)
    {
        target.vector_of_heroes.push_back(std::make_unique<character>(get_random_name(), get_random_alignment()));
    }
}

bool skarabeusz::maze::get_all_paragraph_connections_have_been_processed() const
{
    return std::find_if(vector_of_paragraph_connections.begin(), vector_of_paragraph_connections.end(),[](const auto & pc){ return !pc->get_has_been_processed(); }) == vector_of_paragraph_connections.end();
}

void skarabeusz::generator::calculate_min_distance_to_the_closest_ending()
{
    DEBUG("calculate min distance to the closest ending");

    while (!target.get_all_paragraph_connections_have_been_processed())
    {
        DEBUG("processing paragraph connnections");
        bool found = false;
        for (auto & pc: target.vector_of_paragraph_connections)
        {
            if (pc->first.get_distance_has_been_calculated() && !pc->second.get_distance_has_been_calculated())
            {
                pc->second.set_distance_to_the_closest_ending(pc->first.get_distance_to_the_closest_ending()+1);
                pc->set_has_been_processed(true);
                DEBUG(*pc << " has been processed");
                found = true;
            }
            else
            if (!pc->first.get_distance_has_been_calculated() && pc->second.get_distance_has_been_calculated())
            {
                pc->first.set_distance_to_the_closest_ending(pc->second.get_distance_to_the_closest_ending()+1);
                pc->set_has_been_processed(true);
                DEBUG(*pc << " has been processed");
                found = true;
            }
            else
            if (pc->first.get_distance_has_been_calculated() && pc->second.get_distance_has_been_calculated() && !pc->get_has_been_processed())
            {
                pc->set_has_been_processed(true);
                DEBUG(*pc << " has been processed");
                found = true;
            }
            else
            if (pc->first.get_distance_has_been_calculated() && pc->second.get_distance_has_been_calculated() && pc->first.distance_to_the_closest_ending + 1 < pc->second.distance_to_the_closest_ending)
            {
                pc->second.distance_to_the_closest_ending = pc->first.distance_to_the_closest_ending + 1;
            }
            else
            if (pc->first.get_distance_has_been_calculated() && pc->second.get_distance_has_been_calculated() && pc->second.distance_to_the_closest_ending + 1 < pc->first.distance_to_the_closest_ending)
            {
                pc->first.distance_to_the_closest_ending = pc->second.distance_to_the_closest_ending + 1;
            }
        }

        if (!found)
        {
            for (auto & pc: target.vector_of_paragraph_connections)
            {
                if (!pc->get_has_been_processed())
                {
                    std::cout << *pc << " has not been processed\n";
                }
            }
            for (auto & p: target.vector_of_paragraphs)
            {
                std::cout << *p << " distance " << p->distance_to_the_closest_ending << "\n";
            }
            throw std::runtime_error("not all paragraphs are connected!");
        }
    }
}

void skarabeusz::hyperlink::print_text(std::ostream & s) const
{
    s << "(" << my_paragraph.get_number() << ")";
}


void skarabeusz::story_interpretation::fact_interpretation::print_latex(std::ostream & s) const
{

}

void skarabeusz::story_interpretation::fact_interpretation::print_html(std::ostream & s) const
{
    char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];

    switch (my_fact.fact_paragraph.get_type())
    {
        case paragraph::paragraph_type::CHAMBER_DESCRIPTION:
            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He was in the chamber called %s."), my_maze.vector_of_chambers[my_fact.fact_paragraph.my_state.chamber_id]->get_guardian_family_name().c_str());

            s << buffer;
            break;

        case paragraph::paragraph_type::DISCUSSION:
            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He talked with %s."), my_maze.vector_of_chambers[my_fact.fact_paragraph.my_state.chamber_id]->get_guardian_name().c_str());

            s << buffer;
            break;

        case paragraph::paragraph_type::DOOR:

            if (my_fact.fact_paragraph.vector_of_door_object_indeces.size()>1)
            {
                // TODO - do it even for six door!
                /*
                std::cout << "size of the vector_of_door_object_indeces "
                    << my_fact.fact_paragraph.vector_of_door_object_indeces.size() << "\n";

                std::cout << "door object index 0 "
                    << my_fact.fact_paragraph.vector_of_door_object_indeces[0] << "\n";

                std::cout << "door object index 1 "
                    << my_fact.fact_paragraph.vector_of_door_object_indeces[1] << "\n";
                */

                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He was standing in front of the door %s and %s."),
                my_maze.vector_of_door_objects[my_fact.fact_paragraph.vector_of_door_object_indeces[0]]->get_name().c_str(),
                my_maze.vector_of_door_objects[my_fact.fact_paragraph.vector_of_door_object_indeces[1]]->get_name().c_str()
                );
            }
            else
            if (my_fact.fact_paragraph.vector_of_door_object_indeces.size()==1)
            {
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He was standing in front of the %s door."), my_maze.vector_of_door_objects[my_fact.fact_paragraph.vector_of_door_object_indeces[0]]->get_name().c_str());
            }
            s << buffer;
            break;
    }

    s << " ";
}


void skarabeusz::story_interpretation::print_latex(std::ostream & s) const
{
    for (auto & fi: vector_of_facts_interpretations)
    {
        fi->print_latex(s);
    }
}


void skarabeusz::story_interpretation::print_html_for_transition(std::ostream & s, const fact_interpretation & former, const fact_interpretation & coming) const
{
    char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
    paragraph & p1{former.my_fact.fact_paragraph};
    paragraph & p2{coming.my_fact.fact_paragraph};

    if (p1.get_type() == paragraph::paragraph_type::DOOR && p2.get_type() == paragraph::paragraph_type::DOOR)
    {
        if (p1.my_state.chamber_id != p2.my_state.chamber_id)
        {
            //if (p2.my_state.chamber_id==0) throw std::runtime_error("inconsistency");

            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He opened the door with one of the keys and went through it to the chamber %s."), my_maze.vector_of_chambers[p2.my_state.chamber_id]->my_name.c_str());

            s << buffer;
        }
        else
        if (p1.my_state.my_keys != p2.my_state.my_keys)
        {
            s << my_maze.get_keys_description_in_third_person(p2.my_state.my_keys);
        }
    }
    else
    if (p1.get_type() == paragraph::paragraph_type::DISCUSSION && p2.get_type() == paragraph::paragraph_type::DOOR)
    {
        s << _("He left the dwarf and approached the door.");
    }
    else
    if (p1.get_type() == paragraph::paragraph_type::DOOR && p2.get_type() == paragraph::paragraph_type::DISCUSSION)
    {
        s << _("He approached the dwarf.");
    }
    else
    if (p1.get_type() == paragraph::paragraph_type::DISCUSSION && p2.get_type() == paragraph::paragraph_type::DISCUSSION)
    {
        s << _("He must have exchanged some keys with the dwarf.");
    }
    else
    if (p1.get_type() == paragraph::paragraph_type::DISCUSSION && p2.get_type() == paragraph::paragraph_type::CHAMBER_DESCRIPTION)
    {
        s << _("He left the dwarf and examined the chamber.");
    }
    else
    if (p1.get_type() == paragraph::paragraph_type::CHAMBER_DESCRIPTION && p2.get_type() == paragraph::paragraph_type::DISCUSSION)
    {
        s << _("He stop examining the chamber and approached the dwarf.");
    }
    else
    if (p1.get_type() == paragraph::paragraph_type::DOOR && p2.get_type() == paragraph::paragraph_type::CHAMBER_DESCRIPTION)
    {
        s << _("He approached the dwarf.");
    }
    else
    if (p1.get_type() == paragraph::paragraph_type::CHAMBER_DESCRIPTION && p2.get_type() == paragraph::paragraph_type::DOOR)
    {
        if (p2.vector_of_door_object_indeces.size() == 0) throw std::runtime_error("inconsistency");

        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He approached the door called '%s'."), my_maze.vector_of_door_objects[p2.vector_of_door_object_indeces[0]]->get_key_name().c_str());

        s << buffer;
    }
    else
    if (p1.get_type() == paragraph::paragraph_type::CHAMBER_DESCRIPTION && p2.get_type() == paragraph::paragraph_type::CHAMBER_DESCRIPTION)
    {
        //if (p2.my_state.chamber_id==0) throw std::runtime_error("inconsistency");

        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He was examining the chamber %s."), my_maze.vector_of_chambers[p2.my_state.chamber_id]->my_name.c_str());

        s << buffer;
    }

    s << " ";
}


void skarabeusz::story_interpretation::print_html(std::ostream & s) const
{
    char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];

    if (does_the_teller_like_the_author)
    {
        if (my_maze.vector_of_stories[my_story_index]->story_author == story_teller)
        {
            s << _("I want to tell you something very important.");
        }
        else
        {
            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("Let me tell you a story told by %s author %s."), story_teller.get_favourite_positive_adjective().c_str(), my_maze.vector_of_stories[my_story_index]->story_author.get_name().c_str());

            s << buffer;
        }
    }
    else
    {
        s << _("Let me tell you a story.");
    }
    s << " ";

    s << "<p><i>";

    if (does_the_teller_like_the_hero)
    {
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("Once upon a time there was a %s dwarf named %s."), story_teller.get_favourite_positive_adjective().c_str(), my_maze.vector_of_stories[my_story_index]->story_hero.get_name().c_str());

        s << buffer;
    }
    else
    {
        s << _("Once upon a time there was a dwarf whose name I do not remember.");
    }

    s << " ";

    s << _("He wanted to solve the Maze mystery.");

    s << " ";

    if (vector_of_facts_interpretations.size() == 0) // this should never happen
    {
        s << _("You know what? Maybe I will finish telling you the story some other time.");
        s << " ";
        return;
    }

    int previous_index = 0;

    s << my_maze.get_keys_description_in_third_person(vector_of_facts_interpretations[0]->my_fact.fact_paragraph.my_state.my_keys);

    s << my_maze.get_chamber_name_in_third_person(*my_maze.vector_of_chambers[vector_of_facts_interpretations[0]->my_fact.fact_paragraph.my_state.chamber_id]);

    //std::cout << "This story has " << vector_of_facts_interpretations.size() << " facts, originally " << my_maze.vector_of_stories[my_story_index]->get_amount_of_facts() << "\n";

    for (int i=1; i<vector_of_facts_interpretations.size(); i++)
    {
        print_html_for_transition(s, *vector_of_facts_interpretations[previous_index], *vector_of_facts_interpretations[i]);
        previous_index = i;
        s << " ";

        vector_of_facts_interpretations[i]->print_html(s);
    }

    if (vector_of_facts_interpretations.size() > 1)
    {
        //vector_of_facts_interpretations[vector_of_facts_interpretations.size()-1]->print_html(s);

        s << my_maze.get_keys_description_in_third_person(vector_of_facts_interpretations[vector_of_facts_interpretations.size()-1]->my_fact.fact_paragraph.my_state.my_keys);
    }

    s << _("Then he found a map of the Maze.") << " "
    << _("He studied it carefully for many hours.") << " ";

    snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("By the end he realized that he was %i steps from the closest ending."), vector_of_facts_interpretations[vector_of_facts_interpretations.size()-1]->my_fact.fact_paragraph.get_distance_to_the_closest_ending());
    s << buffer;

    s << "</i></p>";

    s << _("This is the end of the story. Now let us switch to business.");

}

void skarabeusz::story_interpretation::print_text(std::ostream & s) const
{
}

void skarabeusz::paragraph::print_text(std::ostream & s) const
{
    s << number << "\n";
    for (const auto & pi: list_of_paragraph_items)
    {
        pi->print_text(s);
        s << " ";
    }
    s << "\n";
}


skarabeusz::paragraph& skarabeusz::maze::get_discussion_paragraph(chamber_and_keys & c)
{
    auto x = std::find_if(vector_of_paragraphs.begin(), vector_of_paragraphs.end(), [&c](const auto & p){ return p->get_type() == paragraph::paragraph_type::DISCUSSION && p->get_state() == c; });

    if (x == vector_of_paragraphs.end())
    {
        throw std::runtime_error("found no paragraph in this chamber");
    }

    return **x;
}

skarabeusz::paragraph& skarabeusz::maze::get_description_paragraph(chamber_and_keys & c)
{
    auto x = std::find_if(vector_of_paragraphs.begin(), vector_of_paragraphs.end(), [&c](const auto & p){ return p->get_type() == paragraph::paragraph_type::CHAMBER_DESCRIPTION && p->get_state() == c; });

    if (x == vector_of_paragraphs.end())
    {
        throw std::runtime_error("found no paragraph in this chamber");
    }

    return **x;
}

skarabeusz::paragraph& skarabeusz::maze::get_door_paragraph(chamber_and_keys & c, int index)
{
    int amount=0;
    for (int i=0; i<vector_of_paragraphs.size(); i++)
    {
        auto & p=vector_of_paragraphs[i];
        if (p->get_type() == paragraph::paragraph_type::DOOR && p->get_state() == c)
        {
            if (amount == index)
            {
                return *p;
            }
            else
            {
                amount++;
            }
        }
    }
    throw std::runtime_error("index out of range");
}

bool skarabeusz::room::get_has_door_object(door_object & d) const
{
    return std::find_if(list_of_door.begin(), list_of_door.end(),[&d](const auto &my_door)
    { return my_door->get_door_object()==d; }) != list_of_door.end();
}


skarabeusz::paragraph& skarabeusz::maze::get_door_paragraph(chamber_and_keys & c, door_object & d)
{
    auto x = std::find_if(vector_of_paragraphs.begin(), vector_of_paragraphs.end(),
                          [this,&c,&d](const auto & p)
    {
        return p->get_type() == paragraph::paragraph_type::DOOR
                && p->get_state() == c
                && array_of_rooms[p->get_x()][p->get_y()][p->get_z()]->get_has_door_object(d);
    });

    if (x == vector_of_paragraphs.end())
    {
        throw std::runtime_error("found no door paragraph in this chamber");
    }

    return **x;
}


int skarabeusz::maze::get_amount_of_door_in_chamber(chamber_and_keys & c)
{
    int amount=0;
    for (int i=0; i<vector_of_paragraphs.size(); i++)
    {
        auto & p=vector_of_paragraphs[i];
        if (p->get_type() == paragraph::paragraph_type::DOOR && p->get_state() == c)
        {
            amount++;
        }
    }
    return amount;
}


void skarabeusz::generator::generate_names()
{
    for (unsigned chamber_id=1; chamber_id<=target.amount_of_chambers; chamber_id++)
    {
        target.vector_of_chambers[chamber_id-1]->set_guardian_name(get_random_name(), get_random_alignment());
    }
}

const std::string skarabeusz::generator::positive_adjectives_good_perspective[]=
{
    gettext_noop("good"),
    gettext_noop("nice"),
    gettext_noop("honest")
};

const std::string skarabeusz::generator::negative_adjectives_good_perspective[]=
{
    gettext_noop("greedy"),
    gettext_noop("evil"),
    gettext_noop("bad")
};

const std::string skarabeusz::generator::positive_adjectives_bad_perspective[]=
{
    gettext_noop("smart"),
    gettext_noop("intelligent"),
    gettext_noop("gifted")
};

const std::string skarabeusz::generator::negative_adjectives_bad_perspective[]=
{
    gettext_noop("stupid"),
    gettext_noop("lazy"),
    gettext_noop("foolish")
};



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

const std::string skarabeusz::generator::with_the_x_key_names[]=
{
    gettext_noop("with the red key"),
    gettext_noop("with the white key"),
    gettext_noop("with the black key"),
    gettext_noop("with the yellow key"),
    gettext_noop("with the green key"),
    gettext_noop("with the blue key"),
    gettext_noop("with the purple key"),
    gettext_noop("with the diamond key"),
    gettext_noop("with the silver key"),
    gettext_noop("with the golden key"),
    gettext_noop("with the orange key"),
    gettext_noop("with the brown key"),
    gettext_noop("with the gray key"),
    gettext_noop("with the olive key"),
    gettext_noop("with the maroon key"),
    gettext_noop("with the violet key"),
    gettext_noop("with the magenta key"),
    gettext_noop("with the cream key"),
    gettext_noop("with the coral key"),
    gettext_noop("with the burgundy key"),
    gettext_noop("with the peach key"),
    gettext_noop("with the rust key"),
    gettext_noop("with the pink key"),
    gettext_noop("with the cyan key"),
    gettext_noop("with the scarlet key")
};


const std::string skarabeusz::generator::key_names[]=
{
    gettext_noop("red"),
    gettext_noop("white"),
    gettext_noop("black"),
    gettext_noop("yellow"),
    gettext_noop("green"),
    gettext_noop("blue"),
    gettext_noop("purple"),
    gettext_noop("diamond"),
    gettext_noop("silver"),
    gettext_noop("golden"),
    gettext_noop("orange"),
    gettext_noop("brown"),
    gettext_noop("gray"),
    gettext_noop("olive"),
    gettext_noop("maroon"),
    gettext_noop("violet"),
    gettext_noop("magenta"),
    gettext_noop("cream"),
    gettext_noop("coral"),
    gettext_noop("burgundy"),
    gettext_noop("peach"),
    gettext_noop("rust"),
    gettext_noop("pink"),
    gettext_noop("cyan"),
    gettext_noop("scarlet")
};


void skarabeusz::generator::get_random_key_name(std::string & normal_form, std::string & with_the_key_form)
{
    std::vector<std::string> temporary_vector_of_names, temporary_vector_of_with_the_key_forms;
    
    
    unsigned i=0;
    for (auto x:key_names)
    {
        if (map_name_to_taken_flag.find(x)==map_name_to_taken_flag.end())
        {
            temporary_vector_of_names.push_back(x);
            temporary_vector_of_with_the_key_forms.push_back(with_the_x_key_names[i]);
        }
        i++;
    }
    if (temporary_vector_of_names.size()==0)
    {
        throw std::runtime_error("not enough key names");
    }
    std::uniform_int_distribution<unsigned> distr(0, temporary_vector_of_names.size()-1);            
    unsigned index = distr(get_random_number_generator());
    
    auto [it,success]=map_name_to_taken_flag.insert(std::pair(temporary_vector_of_names[index], true));
    if (!success)
    {
        throw std::runtime_error("failed to insert key name");
    }
    
	normal_form = temporary_vector_of_names[index];
    with_the_key_form = temporary_vector_of_with_the_key_forms[index];
}


skarabeusz::character::alignment_type skarabeusz::generator::get_random_alignment()
{
    std::uniform_int_distribution<unsigned> distr(0, 1);
    unsigned index = distr(get_random_number_generator());

    if (index == 0)
        return character::alignment_type::BAD;

    return character::alignment_type::GOOD;
}


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


void skarabeusz::paragraph::print_html(std::ostream & s) const
{
    if (bootstrap)
    {
        s << "<div class=\"container-fluid\">\n";

        s << "<div class=\"row\">\n";
        s << "<div class=\"col-12\">\n";
        s << "<div class=\"bokor-regular\" style=\"font-size:100px; text-align:center;\">Skarabeusz</div>";
        s << "</div>\n";
        s << "</div>\n";

        s << "<div class=\"row\">\n";

        s << "<div class=\"col-2\">\n";

        s << "<h3>" << get_number() << "</h3>\n";

        s << "<img class=\"img-fluid\" src=\"compass.png\">\n";

        s << "</div>";


        s << "<div class=\"col-8\">\n";

        s << "<img src=\"map_" << z << "_" << (get_number()-1) << ".png\" class=\"img-fluid\">\n";

        s << "</div>";

        s << "<div class=\"col-2\">\n";

        s << "</div>\n";

        s << "</div>\n";

        s << "<div class=\"row\">\n";

        s << "<div class=\"col-12\">\n";

        s << "<div class=\"skarabeusz\">\n";

        for (auto & a: list_of_paragraph_items)
        {
            a->print_html(s);
            s << " \n";
        }
        s << "</div>\n";

        s << "</div>\n";

        s << "</div>\n";

        s << "</div>\n";
    }
    else
    {
    s << "<h3>" << get_number() << "</h3>\n";
    
    s << "<img src=\"map_" << z << "_" << (get_number()-1) << ".png\"><img class=\"compass\" src=\"compass.png\" width=\"100\" height=\"100\">\n<br/>\n";
    
    s << "<div class=\"skarabeusz\">\n";
    
    for (auto & a: list_of_paragraph_items)
    {
        a->print_html(s);
        s << " \n";
    }    
    s << "</div>\n";

    }
}


void skarabeusz::paragraph::print_latex(std::ostream & s) const
{
    s << "\\begin{normalsize}\n";
	s << "\\textbf{\\hypertarget{par " << number << "}{" << number << "}}\n";
    
    for (auto & a: list_of_paragraph_items)
    {
        a->print_latex(s);
        s << "\n";
    }
    s << "\\\\\n";    
    
    s << "\\end{normalsize}\n";
    s << "\\\\\n";
}


std::string skarabeusz::chamber::get_description(const maze & m) const
{
    std::stringstream result;
    unsigned x1,y1,z1,x2,y2,z2;
    x1 = m.get_x1_of_chamber(id);
    y1 = m.get_y1_of_chamber(id);
    z1 = m.get_z1_of_chamber(id);
    x2 = m.get_x2_of_chamber(id);
    y2 = m.get_y2_of_chamber(id);
    z2 = m.get_z2_of_chamber(id);
    
    if (x2-x1==1 && y2-y1==1)
    {
        result << _("You are standing in a small room.");
    }
    else
    if (x2-x1==0)
    {
        result << _("You are standing in a corridor leading from north to south.");
    }
    else
    if (y2-y1==0)
    {
        result << _("You are standing in a corridor leading from east to west.");
    }
    else
    if (x2-x1<=3 && y2-y1<=3)
    {
        if (x2-x1==y2-y1)
        {
            result << _("You are standing in a small square room.");
        }
        else
        {
            result << _("You are standing in a small room.");
        }
    }
    else
    {
        if (x2-x1==y2-y1)
        {
            result << _("You are standing in a large square room.");            
        }
        else
        {
            result << _("You are standing in a large room.");
        }
    }
    
    std::string where;
        
    for (unsigned x=x1; x<=x2; x++)
    {
        for (unsigned y=y1; y<=y2; y++)
        {
            if (m.array_of_rooms[x][y][z1]->get_is_seed_room())
            {
                where = m.array_of_rooms[x][y][z1]->get_name();
            }
        }
    }
    
    char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
    snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("Apart from you there is a dwarf at %s here."), where.c_str());
    result << buffer;
    
    result << m.get_wall_description(id, room::direction_type::NORTH)
            << m.get_wall_description(id, room::direction_type::EAST)
            << m.get_wall_description(id, room::direction_type::SOUTH)
            << m.get_wall_description(id, room::direction_type::WEST)
            << m.get_wall_description(id, room::direction_type::UP)
            << m.get_wall_description(id, room::direction_type::DOWN);
            
            
    return result.str();
}

skarabeusz::door & skarabeusz::room::get_door_leading(direction_type d) const
{
    return **std::find_if(list_of_door.begin(),list_of_door.end(),[d](auto & x){ return x->get_direction()==d; });
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
                    name = array_of_rooms[x][y1][z1]->get_name();
                }
            }
            if (amount==0) return "";            
            
            if (amount == 1)
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("There is a door %s in the northern wall."), name.c_str());
                result << buffer;
            }
            else
            {                                
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, ngettext("There are %i door in the northern wall.", "There are %i door in the northern wall.", amount), amount);
                result << buffer;
                                                
                for (unsigned x=x1;x<=x2; x++)
                {
                    if (array_of_rooms[x][y1][z1]->get_has_door_leading(room::direction_type::NORTH))
                    {
                        result << array_of_rooms[x][y1][z1]->get_name() << " ";
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
                    name = array_of_rooms[x2][y][z1]->get_name();
                }
            }            
            if (amount==0) return "";            
            
            if (amount == 1)
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("There is a door %s in the eastern wall."), name.c_str());
                result << buffer;
            }
            else
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, ngettext("There are %i door in the eastern wall.", "There are %i door in the eastern wall.", amount), amount);
                result << buffer;
                
                for (unsigned y=y1;y<=y2; y++)
                {
                    if (array_of_rooms[x2][y][z1]->get_has_door_leading(room::direction_type::EAST))
                    {
                        result << array_of_rooms[x2][y][z1]->get_name() << " ";
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
                    name = array_of_rooms[x][y2][z1]->get_name();
                }
            }
            if (amount==0) return "";
            
            if (amount == 1)
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("There is a door %s in the southern wall."), name.c_str());
                result << buffer;                
            }
            else
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, ngettext("There are %i door in the southern wall.", "There are %i door in the southern wall.", amount), amount);
                result << buffer;
                for (unsigned x=x1;x<=x2; x++)
                {
                    if (array_of_rooms[x][y2][z1]->get_has_door_leading(room::direction_type::SOUTH))
                    {
                        result << array_of_rooms[x][y2][z1]->get_name() << " ";                        
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
                    name = array_of_rooms[x1][y][z1]->get_name();
                }
            }                        
            if (amount==0) return "";            
            
            if (amount == 1)
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("There is a door %s in the western wall."), name.c_str());
                result << buffer;                
            }
            else
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, ngettext("There are %i door in the western wall.", "There are %i door in the western wall.", amount), amount);
                result << buffer;
                for (unsigned y=y1;y<=y2; y++)
                {
                    if (array_of_rooms[x1][y][z1]->get_has_door_leading(room::direction_type::WEST))
                    {
                        result << array_of_rooms[x1][y][z1]->get_name() << " ";
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
                        name = array_of_rooms[x][y][z2]->get_name();
                    }
                }
            }                        
            if (amount==0) return "";            
            
            if (amount == 1)
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("There is a hatch %s in the ceiling."), name.c_str());
                result << buffer;                
            }
            else
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, ngettext("There is %i hatch in the ceiling.", "There are %i hatches in the ceiling.", amount), amount);
                result << buffer;
                
                for (unsigned x=x1;x<=x2; x++)
                {
                    for (unsigned y=y1;y<=y2; y++)
                    {
                        if (array_of_rooms[x][y][z1]->get_has_door_leading(room::direction_type::UP))
                        {
                            result << array_of_rooms[x][y][z1]->get_name() << " ";
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
                        name = array_of_rooms[x][y][z2]->get_name();
                    }
                }
            }                        
            if (amount==0) return "";            
            
            if (amount == 1)
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("There is a hatch %s in the floor."), name.c_str());
                result << buffer;                
            }
            else
            {
                char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, ngettext("There is %i hatch in the floor.", "There are %i hatches in the floor.", amount), amount);
                result << buffer;
                
                for (unsigned x=x1;x<=x2; x++)
                {
                    for (unsigned y=y1;y<=y2; y++)
                    {
                        if (array_of_rooms[x][y][z1]->get_has_door_leading(room::direction_type::DOWN))
                        {
                            result << array_of_rooms[x][y][z1]->get_name() << " ";
                        }
                    }
                }
                result << ". ";
            }
            break;
    }    
    
    return result.str();
}


skarabeusz::door_object & skarabeusz::maze::get_door_object(unsigned chamber1, unsigned chamber2)
{
    for (auto & d: vector_of_door_objects)
    {
        if (d->get_connects(chamber1, chamber2))
        {
            return *d;
        }
    }
    throw std::runtime_error("unable to find the door object");
}

const skarabeusz::door_object & skarabeusz::maze::get_door_object(unsigned chamber1, unsigned chamber2) const
{
    for (auto & d: vector_of_door_objects)
    {
        if (d->get_connects(chamber1, chamber2))
        {
            return *d;
        }
    }
    throw std::runtime_error("unable to find the door object");
}


std::string skarabeusz::maze::get_keys_description_after_exchange(const keys & k) const
{
    std::stringstream result;
    std::stringstream s;
    
    unsigned amount=0;
    unsigned number=0;
    std::string t;
    
    for (unsigned i=0; i<amount_of_chambers; i++)
    {
        for (unsigned j=i+1; j<amount_of_chambers; j++)
        {
            if (k.get_matrix()[i][j]) { amount++; t = get_door_object(i+1,j+1).get_key_name(); }
            number++;
        }
    }
    
    if (amount == 1)
    {
        
        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("The dwarf says: \"I would give you the %s key for your keys.\""), t.c_str());
        result << buffer;
    }
    else
    if (amount == 2)
    {
        number=0;
        std::vector<std::string> vector_of_two_names;
        
        for (unsigned i=0; i<amount_of_chambers; i++)
        {
            for (unsigned j=i+1; j<amount_of_chambers; j++)
            {
                if (k.get_matrix()[i][j]) { vector_of_two_names.push_back(get_door_object(i+1,j+1).get_key_name()); }
                number++;
            }
        }
        
        char buffer0[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer0, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("%s and %s"), vector_of_two_names[0].c_str(), vector_of_two_names[1].c_str());

        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("The dwarf says: \"I would give you the keys %s for your keys.\""), buffer0);
        result << buffer;
    }
    else
    {
        number=0;
        std::vector<std::string> vector_of_key_names;

        for (unsigned i=0; i<amount_of_chambers; i++)
        {
            for (unsigned j=i+1; j<amount_of_chambers; j++)
            {
                if (k.get_matrix()[i][j]) { vector_of_key_names.push_back(get_door_object(i+1,j+1).get_key_name()); }
                number++;
            }
        }


        std::stringstream buffer0;

        for (unsigned int i=0; i<vector_of_key_names.size(); i++)
        {
            buffer0 << vector_of_key_names[i];

            if (i + 1<vector_of_key_names.size())
            {
                buffer0 << ", ";
            }
        }

        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("The dwarf says: \"I would give you the keys %s for your keys.\""), buffer0.str().c_str());
        result << buffer;
    }
    
    return result.str();
}


std::string skarabeusz::maze::get_chamber_name_in_third_person(const chamber & c) const
{
    std::stringstream result;
    char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];

    snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He was in the chamber called %s."), c.my_name.c_str());
    result << buffer;


    return result.str();
}


std::string skarabeusz::maze::get_keys_description_in_third_person(const keys & k) const
{
    std::stringstream result;
    std::stringstream s;
    unsigned amount=0;
    unsigned number=0;
    std::string t;

    for (unsigned i=0; i<amount_of_chambers; i++)
    {
        for (unsigned j=i+1; j<amount_of_chambers; j++)
        {
            if (k.get_matrix()[i][j]) { amount++; t=get_door_object(i+1,j+1).get_key_name(); }
            number++;
        }
    }

    if (amount == 1)
    {
        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He had the %s key."), t.c_str());
        result << buffer;
    }
    else
        if (amount == 2)
        {
            number=0;
            std::vector<std::string> vector_of_two_names;

            for (unsigned i=0; i<amount_of_chambers; i++)
            {
                for (unsigned j=i+1; j<amount_of_chambers; j++)
                {
                    if (k.get_matrix()[i][j]) { vector_of_two_names.push_back(get_door_object(i+1,j+1).get_key_name()); }
                    number++;
                }
            }

            char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He had the keys %s and %s."), vector_of_two_names[0].c_str(), vector_of_two_names[1].c_str());
            result << buffer;
        }
        else
        {
            number=0;
            std::vector<std::string> vector_of_key_names;

            for (unsigned i=0; i<amount_of_chambers; i++)
            {
                for (unsigned j=i+1; j<amount_of_chambers; j++)
                {
                    if (k.get_matrix()[i][j]) { vector_of_key_names.push_back(get_door_object(i+1,j+1).get_key_name()); }
                    number++;
                }
            }

            std::stringstream buffer0;

            for (unsigned int i=0; i<vector_of_key_names.size(); i++)
            {
                buffer0 << vector_of_key_names[i];

                if (i + 1<vector_of_key_names.size())
                {
                    buffer0 << ", ";
                }
            }

            char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("He had the following keys: %s."), buffer0.str().c_str());
            result << buffer;
        }



        return result.str();
}




std::string skarabeusz::maze::get_keys_description(const keys & k) const
{
    std::stringstream result;
    std::stringstream s;
    unsigned amount=0;
    unsigned number=0;
    std::string t;
    
    for (unsigned i=0; i<amount_of_chambers; i++)
    {
        for (unsigned j=i+1; j<amount_of_chambers; j++)
        {
            if (k.get_matrix()[i][j]) { amount++; t=get_door_object(i+1,j+1).get_key_name(); }
            number++;
        }
    }
    
    
    if (amount == 1)
    {
        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You have the %s key."), t.c_str());
        result << buffer;
    }
    else
    if (amount == 2)
    {
        number=0;
        std::vector<std::string> vector_of_two_names;
        
        for (unsigned i=0; i<amount_of_chambers; i++)
        {
            for (unsigned j=i+1; j<amount_of_chambers; j++)
            {
                if (k.get_matrix()[i][j]) { vector_of_two_names.push_back(get_door_object(i+1,j+1).get_key_name()); }
                number++;
            }
        }
        char buffer0[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer0, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("%s and %s"), vector_of_two_names[0].c_str(), vector_of_two_names[1].c_str());
        
        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You have the keys %s."), buffer0);
        result << buffer;
    }
    else
    {
        number=0;
        std::vector<std::string> vector_of_key_names;

        for (unsigned i=0; i<amount_of_chambers; i++)
        {
            for (unsigned j=i+1; j<amount_of_chambers; j++)
            {
                if (k.get_matrix()[i][j]) { vector_of_key_names.push_back(get_door_object(i+1,j+1).get_key_name()); }
                number++;
            }
        }

        std::stringstream buffer0;

        for (unsigned int i=0; i<vector_of_key_names.size(); i++)
        {
            buffer0 << vector_of_key_names[i];

            if (i + 1<vector_of_key_names.size())
            {
                buffer0 << ", ";
            }
        }

        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You have the following keys: %s."), buffer0.str().c_str());
        result << buffer;
    }
    return result.str();
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
    std::stringstream result;
    unsigned chamber_id;
    if (list_of_virtual_door.begin() == list_of_virtual_door.end())
    {
        result << _("The dwarf pays no attention at you.");
    }
    else
    {
        chamber_id = list_of_virtual_door.begin()->get_chamber_id()-1;
        
        DEBUG("collection_of_virtual_door::get_description chamber_id = " << chamber_id);
        DEBUG("x,y,z = " << m.vector_of_chambers[chamber_id]->get_seed().x << "," << m.vector_of_chambers[chamber_id]->get_seed().y << "," << m.vector_of_chambers[chamber_id]->get_seed().z);

        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You are talking with the dwarf at %s."), m.array_of_rooms[m.vector_of_chambers[chamber_id]->get_seed().x][m.vector_of_chambers[chamber_id]->get_seed().y][m.vector_of_chambers[chamber_id]->get_seed().z]->get_name().c_str());
        result << buffer;        
    }
    
    return result.str();
}

bool skarabeusz::room::get_door_can_be_opened_with(direction_type d, const keys & k) const
{
    for (auto &a: list_of_door)
    {
        if (a->get_direction() == d && k.get_matrix()[a->get_chamber1()-1][a->get_chamber2()-1])
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
        if (vector_of_paragraphs[i]->get_state().get_keys() == k && vector_of_paragraphs[i]->get_x() == x && vector_of_paragraphs[i]->get_y() == y && vector_of_paragraphs[i]->get_z() == z && vector_of_paragraphs[i]->get_type() == t)
        {
            return i;
        }
    }
    throw std::runtime_error("internal error could not get paragraph index (1)");
}

unsigned skarabeusz::maze::get_paragraph_index(unsigned chamber_id, const keys & k, paragraph::paragraph_type t) const
{
    for (unsigned i=0; i<vector_of_paragraphs.size(); i++)
    {
        if (vector_of_paragraphs[i]->get_state().get_chamber_id() == chamber_id && vector_of_paragraphs[i]->get_state().get_keys() == k && vector_of_paragraphs[i]->get_type() == t)
        {
            return i;
        }
    }
    throw std::runtime_error("internal error could not get paragraph index (2)");
}

std::string skarabeusz::room::which_key_can_open(direction_type d) const
{
    for (auto &a: list_of_door)
    {
        if (a->get_direction() == d)
        {
            return a->get_door_object().get_with_the_key_name();
        }
    }
    return "";  // this should never happen
}

skarabeusz::paragraph::paragraph(unsigned n, const chamber_and_keys & c, unsigned nx, unsigned ny, unsigned nz, paragraph_type t):
number{n},
my_state{c},
type{t}, x{nx}, y{ny},z{nz}, ending{false},
distance_to_the_closest_ending{std::numeric_limits<int>::max()},
distance_has_been_calculated{false},
bootstrap{false}
{}

void skarabeusz::hint_you_still_need_i_steps::print_latex(std::ostream & s) const
{
    char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
    snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You still need at least %i steps to achieve your target."), my_paragraph.distance_to_the_closest_ending);
    s << buffer;
}

void skarabeusz::hint_you_still_need_i_steps::print_html(std::ostream & s) const
{
    char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
    snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You still need at least %i steps to achieve your target."), my_paragraph.distance_to_the_closest_ending);

    s << buffer;
}

void skarabeusz::hint_you_still_need_i_steps::print_text(std::ostream & s) const
{
    char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
    snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You still need at least %i steps to achieve your target."), my_paragraph.distance_to_the_closest_ending);

    s << buffer;
}


void skarabeusz::paragraph::add_door_object_index(int i)
{
    if (i!=-1 && std::find(vector_of_door_object_indeces.begin(), vector_of_door_object_indeces.end(), i) == vector_of_door_object_indeces.end())
    {
        vector_of_door_object_indeces.push_back(i);
        std::cout << "add door object index " << i << "\n";
    }
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
        
        unsigned x, y, z;
        
        x = target.vector_of_chambers[i.first]->get_seed().x;
        y = target.vector_of_chambers[i.first]->get_seed().y;
        z = target.vector_of_chambers[i.first]->get_seed().z;
        
        target.vector_of_paragraphs.push_back(std::make_unique<paragraph>(++number, c, x, y, z, paragraph::paragraph_type::CHAMBER_DESCRIPTION));
    }    
    
    amount_of_chamber_descriptions = list_of_processed_pairs_chamber_and_keys.size();
    
    for (auto & i: list_of_processed_pairs_chamber_and_keys)
    {
        chamber_and_keys c{i.first, i.second};
        
        unsigned x, y, z;
        
        x = target.vector_of_chambers[i.first]->get_seed().x;
        y = target.vector_of_chambers[i.first]->get_seed().y;
        z = target.vector_of_chambers[i.first]->get_seed().z;
        
        target.vector_of_paragraphs.push_back(std::make_unique<paragraph>(++number, c, x, y, z , paragraph::paragraph_type::DISCUSSION));

        target.vector_of_paragraph_connections.push_back(std::make_unique<paragraph_connection>(target.get_discussion_paragraph(c), target.get_description_paragraph(c)));
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
        while (target.vector_of_paragraphs[index+amount_of_chamber_descriptions]->get_ending());
        
        target.vector_of_paragraphs[index+amount_of_chamber_descriptions]->set_ending(true);

        target.vector_of_paragraphs[index+amount_of_chamber_descriptions]->set_distance_to_the_closest_ending(0);

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
                    if (target.array_of_rooms[x][y][z]->get_list_of_door().size()>0)
                    {
                        auto new_door_paragraph = std::make_unique<paragraph>(++number, c, x,y,z, paragraph::paragraph_type::DOOR);

                        target.vector_of_paragraphs.push_back(std::move(new_door_paragraph));
                        amount_of_door_rooms++;
                    }
                }
            }
        }        
    }
  
    for (auto & i: list_of_processed_pairs_chamber_and_keys)
    {
        chamber_and_keys c{i.first, i.second};

        int amount = target.get_amount_of_door_in_chamber(c);

        for (int j=0; j<amount; j++)
        {
            target.vector_of_paragraph_connections.push_back(std::make_unique<paragraph_connection>(target.get_description_paragraph(c), target.get_door_paragraph(c, j)));

            auto & my_door_paragraph = target.get_door_paragraph(c, j);
            auto & my_room = target.array_of_rooms[my_door_paragraph.get_x()][my_door_paragraph.get_y()][my_door_paragraph.get_z()];

            for (auto & dd: my_room->get_list_of_door())
            {
                for (int d=static_cast<int>(room::direction_type::NORTH); d < 6; d++)
                {
                    if (my_room->get_has_door_leading(static_cast<room::direction_type>(d)) && my_room->get_door_can_be_opened_with(static_cast<room::direction_type>(d), i.second))
                    {
                        if (dd->chamber1 == i.first)
                        {
                            chamber_and_keys c2{dd->chamber2, i.second};

                            auto pc = std::make_unique<paragraph_connection>(target.get_door_paragraph(c, j), target.get_door_paragraph(c2, dd->get_door_object()));

                            pc->set_door_object_index(dd->get_door_object().get_index());

                            target.vector_of_paragraph_connections.push_back(std::move(pc));
                        }
                        else
                        if (dd->chamber2 == i.first)
                        {
                            chamber_and_keys c2{dd->chamber2, i.second};

                            auto pc = std::make_unique<paragraph_connection>(target.get_door_paragraph(c, j), target.get_door_paragraph(c2, dd->get_door_object()));

                            pc->set_door_object_index(dd->get_door_object().get_index());

                            target.vector_of_paragraph_connections.push_back(std::move(pc));
                        }
                    }
                }
            }
        }
    }


    target.amount_of_paragraphs = number;
    
    for (unsigned i=0; i<amount_of_chamber_descriptions; i++)
    {
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.vector_of_chambers[target.vector_of_paragraphs[i]->get_state().get_chamber_id()]->get_description(target)));
    }
    
    number=amount_of_chamber_descriptions + amount_of_discussions;
    
    for (unsigned i=0; i<amount_of_chamber_descriptions; i++)
    {
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.get_keys_description(target.vector_of_paragraphs[i]->get_state().get_keys())));
                
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(_("What do you want to do?")));
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(_("Talk with the dwarf - go to ")));
        target.vector_of_paragraphs[i]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[i+amount_of_chamber_descriptions]));
        target.vector_of_paragraph_connections.push_back(std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[i], *target.vector_of_paragraphs[i+amount_of_chamber_descriptions]));
        
        unsigned x1,x2,y1,y2,z1,z2;
        unsigned chamber_id = target.vector_of_paragraphs[i+amount_of_chamber_descriptions]->get_state().get_chamber_id()+1;
        keys & k=target.vector_of_paragraphs[i+amount_of_chamber_descriptions]->get_state().get_keys();
        
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
                    if (target.array_of_rooms[x][y][z]->get_list_of_door().size()>0)
                    {                        
                        auto current_room{target.array_of_rooms[x][y][z]};

                        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
                        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("Go to the field %s - "), target.array_of_rooms[x][y][z]->get_name().c_str());
                        
                        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(buffer));                        
                        target.vector_of_paragraphs[i]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[number]));

                        target.vector_of_paragraph_connections.push_back(std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[i], *target.vector_of_paragraphs[number]));

                        
                        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You are standing in front of the door at %s - "), target.array_of_rooms[x][y][z]->get_name().c_str());

                        target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(buffer));                        
                        
                        target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(target.get_keys_description(target.vector_of_paragraphs[i]->get_state().get_keys())));

                        for (unsigned d=0; d<6; d++)
                        {
                            if (target.array_of_rooms[x][y][z]->get_has_door_leading(static_cast<room::direction_type>(d)))
                            {
                                room::direction_type di=static_cast<room::direction_type>(d);
                                switch (di)
                                {
                                    case room::direction_type::NORTH:
                                        
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->get_state().get_keys()))
                                        {                                        
                                            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You can open the door %s and go north "), _(target.array_of_rooms[x][y][z]->which_key_can_open(di).c_str()));                                            
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(buffer));                                                                                    
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x,y-1,z,k, paragraph::paragraph_type::DOOR)]));


                                            auto pc = std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[number], *target.vector_of_paragraphs[target.get_paragraph_index(x,y-1,z,k, paragraph::paragraph_type::DOOR)]);

                                            if (!current_room->get_has_door_leading(di))
                                                throw std::runtime_error("inconsistency");

                                            auto &dd=current_room->get_door_leading(di);

                                            pc->set_door_object_index(dd.get_door_object().get_index());

                                            target.vector_of_paragraph_connections.push_back(std::move(pc));

                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(_("None of your keys opens the north door.")));

                                            auto &dd=current_room->get_door_leading(di);

                                            target.vector_of_paragraphs[number]->add_door_object_index(dd.get_door_object().get_index());

                                            //target.vector_of_paragraphs[target.get_paragraph_index(x,y-1,z,k, paragraph::paragraph_type::DOOR)]->add_door_object_index(dd.get_door_object().get_index());
                                        }
                                        break;
                                    case room::direction_type::EAST:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->get_state().get_keys()))
                                        {                                        
                                            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You can open the door %s and go east "), _(target.array_of_rooms[x][y][z]->which_key_can_open(di).c_str()));                                            
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(buffer));                                                                                    
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x+1,y,z,k, paragraph::paragraph_type::DOOR)]));

                                            auto pc = std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[number], *target.vector_of_paragraphs[target.get_paragraph_index(x+1,y,z,k, paragraph::paragraph_type::DOOR)]);


                                            if (!current_room->get_has_door_leading(di))
                                                throw std::runtime_error("inconsistency");

                                            auto &dd=current_room->get_door_leading(di);

                                            pc->set_door_object_index(dd.get_door_object().get_index());

                                            target.vector_of_paragraph_connections.push_back(std::move(pc));

                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(_("None of your keys opens the east door.")));

                                            auto &dd=current_room->get_door_leading(di);

                                            target.vector_of_paragraphs[number]->add_door_object_index(dd.get_door_object().get_index());

                                            //target.vector_of_paragraphs[target.get_paragraph_index(x+1,y,z,k, paragraph::paragraph_type::DOOR)]->add_door_object_index(dd.get_door_object().get_index());

                                        }
                                        break;
                                    case room::direction_type::SOUTH:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->get_state().get_keys()))
                                        {                                        
                                            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You can open the door %s and go south "), _(target.array_of_rooms[x][y][z]->which_key_can_open(di).c_str()));                                            
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(buffer));                                                                                    
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x,y+1,z,k, paragraph::paragraph_type::DOOR)]));

                                            auto pc = std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[number], *target.vector_of_paragraphs[target.get_paragraph_index(x,y+1,z,k, paragraph::paragraph_type::DOOR)]);

                                            if (!current_room->get_has_door_leading(di))
                                                throw std::runtime_error("inconsistency");

                                            auto &dd=current_room->get_door_leading(di);

                                            pc->set_door_object_index(dd.get_door_object().get_index());

                                            target.vector_of_paragraph_connections.push_back(std::move(pc));

                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(_("None of your keys opens the south door.")));

                                            auto &dd=current_room->get_door_leading(di);

                                            target.vector_of_paragraphs[number]->add_door_object_index(dd.get_door_object().get_index());

                                            //target.vector_of_paragraphs[target.get_paragraph_index(x,y+1,z,k, paragraph::paragraph_type::DOOR)]->add_door_object_index(dd.get_door_object().get_index());
                                        }
                                        break;
                                    case room::direction_type::WEST:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->get_state().get_keys()))
                                        {                                        
                                            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You can open the door %s and go west "), _(target.array_of_rooms[x][y][z]->which_key_can_open(di).c_str()));                                            
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(buffer));                                                                                    
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x-1,y,z,k, paragraph::paragraph_type::DOOR)]));


                                            auto pc = std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[number], *target.vector_of_paragraphs[target.get_paragraph_index(x-1,y,z,k, paragraph::paragraph_type::DOOR)]);

                                            if (!current_room->get_has_door_leading(di))
                                                throw std::runtime_error("inconsistency");

                                            auto &dd=current_room->get_door_leading(di);

                                            pc->set_door_object_index(dd.get_door_object().get_index());

                                            target.vector_of_paragraph_connections.push_back(std::move(pc));

                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(_("None of your keys opens the west door.")));

                                            auto &dd=current_room->get_door_leading(di);

                                            target.vector_of_paragraphs[number]->add_door_object_index(dd.get_door_object().get_index());

                                            //target.vector_of_paragraphs[target.get_paragraph_index(x-1,y,z,k, paragraph::paragraph_type::DOOR)]->add_door_object_index(dd.get_door_object().get_index());
                                        }
                                        break;
                                    case room::direction_type::DOWN:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->get_state().get_keys()))
                                        {                                        
                                            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You can open the hatch %s and descend to the lower level "), _(target.array_of_rooms[x][y][z]->which_key_can_open(di).c_str()));                                            
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(buffer));                                                                                    
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x,y,z-1,k, paragraph::paragraph_type::DOOR)]));


                                            auto pc = std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[number], *target.vector_of_paragraphs[target.get_paragraph_index(x,y,z-1,k, paragraph::paragraph_type::DOOR)]);


                                            if (!current_room->get_has_door_leading(di))
                                                throw std::runtime_error("inconsistency");

                                            auto &dd=current_room->get_door_leading(di);

                                            pc->set_door_object_index(dd.get_door_object().get_index());

                                            target.vector_of_paragraph_connections.push_back(std::move(pc));

                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(_("None of your keys opens the hatch in the floor.")));
                                            auto &dd=current_room->get_door_leading(di);

                                            target.vector_of_paragraphs[number]->add_door_object_index(dd.get_door_object().get_index());

                                            //target.vector_of_paragraphs[target.get_paragraph_index(x,y,z-1,k, paragraph::paragraph_type::DOOR)]->add_door_object_index(dd.get_door_object().get_index());
                                        }
                                        break;
                                    case room::direction_type::UP:
                                        if (target.array_of_rooms[x][y][z]->get_door_can_be_opened_with(di, target.vector_of_paragraphs[number]->get_state().get_keys()))
                                        {                                        
                                            snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You can open the hatch %s and ascend to the upper level "), _(target.array_of_rooms[x][y][z]->which_key_can_open(di).c_str()));                                            
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(buffer));                                                                                    
                                            target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(x,y,z+1,k, paragraph::paragraph_type::DOOR)]));

                                            auto pc = std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[number], *target.vector_of_paragraphs[target.get_paragraph_index(x,y,z+1,k, paragraph::paragraph_type::DOOR)]);

                                            if (!current_room->get_has_door_leading(di))
                                                throw std::runtime_error("inconsistency");

                                            auto &dd=current_room->get_door_leading(di);

                                            pc->set_door_object_index(dd.get_door_object().get_index());

                                            target.vector_of_paragraph_connections.push_back(std::move(pc));

                                        }
                                        else
                                        {
                                            target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(_("None of your keys opens the hatch in the ceiling.")));
                                            auto &dd=current_room->get_door_leading(di);

                                            target.vector_of_paragraphs[number]->add_door_object_index(dd.get_door_object().get_index());

                                            //target.vector_of_paragraphs[target.get_paragraph_index(x,y,z+1,k, paragraph::paragraph_type::DOOR)]->add_door_object_index(dd.get_door_object().get_index());
                                        }
                                        break;                                        
                                }
                            }
                        }
                        
                        target.vector_of_paragraphs[number]->add(std::make_unique<normal_text>(_("You can look around ")));
                        target.vector_of_paragraphs[number]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(target.vector_of_paragraphs[number]->get_state().get_chamber_id(),target.vector_of_paragraphs[number]->get_state().get_keys(), paragraph::paragraph_type::CHAMBER_DESCRIPTION)]));
                                                

                        target.vector_of_paragraph_connections.push_back(std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[number], *target.vector_of_paragraphs[target.get_paragraph_index(target.vector_of_paragraphs[number]->get_state().get_chamber_id(),target.vector_of_paragraphs[number]->get_state().get_keys(), paragraph::paragraph_type::CHAMBER_DESCRIPTION)]));

                        number++;                        
                    }
                }
            }
        }        
    }
    
    DEBUG("amount of chamber descriptions " << amount_of_chamber_descriptions);
    
    for (unsigned i=amount_of_chamber_descriptions; i<amount_of_chamber_descriptions+amount_of_discussions; i++)
    {
        DEBUG("get from chamber " << target.vector_of_paragraphs[i]->get_state().get_chamber_id());        
        unsigned chamber_id = target.vector_of_paragraphs[i]->get_state().get_chamber_id()+1;
        keys & k=target.vector_of_paragraphs[i]->get_state().get_keys();
                
        auto it = std::find_if(target.vector_of_virtual_door[target.vector_of_paragraphs[i]->get_state().get_chamber_id()].get_list_of_virtual_door().begin(),
                     target.vector_of_virtual_door[target.vector_of_paragraphs[i]->get_state().get_chamber_id()].get_list_of_virtual_door().end(),
                     [&k](auto & x){return x.get_contains(k.get_matrix());});
        

        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("You are standing next to the dwarf at %s."), target.array_of_rooms[target.vector_of_chambers[chamber_id-1]->get_seed().x][target.vector_of_chambers[chamber_id-1]->get_seed().y][target.vector_of_chambers[chamber_id-1]->get_seed().z]->get_name().c_str());
        
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(buffer));
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.get_keys_description(target.vector_of_paragraphs[i]->get_state().get_keys())));
        
        bool exchange = false;
        
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(_("Hello, my name is ")));
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(target.vector_of_chambers[chamber_id-1]->get_guardian_name()));
        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(". "));
        
        if (target.vector_of_paragraphs[i]->get_ending())
        {
            target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(_("Congratulations! You have won!")));
        }        
        else
        {
            if (parameters.stories)
            {
                target.vector_of_paragraphs[i]->add(std::make_unique<story_interpretation>(target, target.vector_of_chambers[chamber_id-1]->get_guardian_alignment(), *target.vector_of_paragraphs[i],
                *target.vector_of_chambers[chamber_id-1]->my_guardian));
            }

            if (parameters.hints)
            {
                target.vector_of_paragraphs[i]->add(std::make_unique<hint_you_still_need_i_steps>(*target.vector_of_paragraphs[i]));

                snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("There are %i possible endings."), parameters.amount_of_alternative_endings);

                target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(buffer));
            }



            if (it != target.vector_of_virtual_door[target.vector_of_paragraphs[i]->get_state().get_chamber_id()].get_list_of_virtual_door().end())
            {
                for (auto x=it->get_list_of_matrices().begin(); x!=it->get_list_of_matrices().end(); x++)
                {
                    keys k2{*x};
                    if (k != k2)
                    {
                        std::string t = it->get_description(target, x);                
                        exchange=true;
                        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(t));
                        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(_("If you accept it go to ")));                                        
                        target.vector_of_paragraphs[i]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(target.vector_of_paragraphs[i]->get_state().get_chamber_id(), k2, paragraph::paragraph_type::DISCUSSION)]));
                        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(". "));


                        target.vector_of_paragraph_connections.push_back(std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[i], *target.vector_of_paragraphs[target.get_paragraph_index(target.vector_of_paragraphs[i]->get_state().get_chamber_id(), k2, paragraph::paragraph_type::DISCUSSION)]));

                    }
                }
            }
            if (!exchange)
            {
                target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(_("The dwarf does not accept any exchange.")));            
            }
        }

        target.vector_of_paragraphs[i]->add(std::make_unique<normal_text>(_("You can look around ")));
        target.vector_of_paragraphs[i]->add(std::make_unique<hyperlink>(*target.vector_of_paragraphs[target.get_paragraph_index(target.vector_of_paragraphs[i]->get_state().get_chamber_id(),target.vector_of_paragraphs[i]->get_state().get_keys(), paragraph::paragraph_type::CHAMBER_DESCRIPTION)]));        


        target.vector_of_paragraph_connections.push_back(std::make_unique<paragraph_connection>(*target.vector_of_paragraphs[i], *target.vector_of_paragraphs[target.get_paragraph_index(target.vector_of_paragraphs[i]->get_state().get_chamber_id(),target.vector_of_paragraphs[i]->get_state().get_keys(), paragraph::paragraph_type::CHAMBER_DESCRIPTION)]));

    }
    
    std::shuffle(target.vector_of_paragraphs.begin(), target.vector_of_paragraphs.end(), gen);
    
    for (unsigned i=0; i<target.vector_of_paragraphs.size(); i++)
    {
        target.vector_of_paragraphs[i]->set_number(i+1);
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
	if (p1.get_keys() == p2.get_keys() && p1.get_keys().get_matrix()[p1.get_chamber_id()-1][p2.get_chamber_id()-1])
		return true;

	// the chambers are equal and the keys can be exchanged
	if (p1.get_chamber_id() == p2.get_chamber_id() && target.vector_of_virtual_door[p1.get_chamber_id()-1].get_can_exchange(p1.get_keys().get_matrix(), p2.get_keys().get_matrix()))
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
	virtual_door d{a->get_chamber_id()};
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
		chamber_and_keys c((*i).first+1, (*i).second.get_matrix());
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
        //DEBUG("amount of pairs that have been done " << get_amount_of_pairs_that_have_been_done() << "/" << vector_of_pairs_chamber_and_keys.size());
        
		bool s = process_a_journey_step();

		if (!get_all_pairs_have_been_done() && !s)
		{
			// find a pair that has not been done, but a chamber that has
			unsigned candidate_not_done = 0, candidate_done = 0;

			process_a_journey_get_candidate(candidate_done, candidate_not_done);

			unsigned c = vector_of_pairs_chamber_and_keys[candidate_done].get_chamber_id();

			target.vector_of_virtual_door[c-1].merge_classes(vector_of_pairs_chamber_and_keys[candidate_not_done].get_keys().get_matrix(), 
                                                                  vector_of_pairs_chamber_and_keys[candidate_done].get_keys().get_matrix());

			if (!process_a_journey_step())
				throw std::runtime_error("internal error - cannot process a journey");
		}
	}
	while (!get_all_pairs_have_been_done());
}



void skarabeusz::generator::process_the_equivalence_classes()
{
    double e=list_of_processed_pairs_chamber_and_keys.size();
    double f=e*e;
    unsigned long f0=0l;
    
    
    
	for (std::list<std::pair<unsigned, keys>>::iterator i(list_of_processed_pairs_chamber_and_keys.begin()); i != list_of_processed_pairs_chamber_and_keys.end(); i++)
	{
		for (std::list<std::pair<unsigned, keys>>::iterator j(list_of_processed_pairs_chamber_and_keys.begin()); j != list_of_processed_pairs_chamber_and_keys.end(); j++)
		{
            f0++;
            std::cout << "done " << f0 << " all " << f << "\n";
            
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
				d.push_back((*k).get_matrix());
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
                                                
                if (k.get_matrix()[chamber1-1][chamber2-1])
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
                                if (k2.get_matrix()[a-1][b-1])
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
                                k2.get_matrix()[a][b]=true;
                                k2.get_matrix()[b][a]=true;
                            }
                            
                            //k2.report(std::cout);
                            
                            if (std::find(list_of_keys.begin(),list_of_keys.end(), k2)==list_of_keys.end())
                            {
                                if (k2.get_amount_of_keys() != 0 && k2.get_amount_of_keys()<=parameters.max_amount_of_keys_to_hold && k2.get_matrix()[chamber1-1][chamber2-1])
                                {
                                    list_of_keys.push_back(k2);
                                    DEBUG("add a combination of keys");
                                }
                                else
                                {
                                    DEBUG("could not add a combination, amount of keys " << k2.get_amount_of_keys() << " transition from " << chamber1 << " to " << chamber2 << " equals " << (k2.get_matrix()[chamber1-1][chamber2-1] ? "1" : "0"));
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
                unsigned x1=r1->get_x(),y1=r1->get_y(),z1=r1->get_z();
                for (unsigned d=0; d<6; d++)
                {
                    switch (static_cast<room::direction_type>(d))
                    {
                        case room::direction_type::NORTH:
                            if (y1 > 0)
                            {
                                if (array_of_rooms[x1][y1-1][z1]->get_chamber_id()==chamber2)
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
                                if (array_of_rooms[x1+1][y1][z1]->get_chamber_id()==chamber2)
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
                                if (array_of_rooms[x1][y1+1][z1]->get_chamber_id()==chamber2)
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
                                if (array_of_rooms[x1-1][y1][z1]->get_chamber_id()==chamber2)
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
                                if (array_of_rooms[x1][y1][z1+1]->get_chamber_id()==chamber2)
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
                                if (array_of_rooms[x1][y1][z1-1]->get_chamber_id()==chamber2)
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

                std::string normal_form, with_the_key_form;
                g.get_random_key_name(normal_form, with_the_key_form);
                
                std::unique_ptr<door_object> dx=std::make_unique<door_object>(r1->get_chamber_id(), r2->get_chamber_id(), ++amount_of_keys, _(normal_form.c_str()), _(with_the_key_form.c_str()));
                
                r1->get_list_of_door().push_back(std::make_shared<door>(r1->get_chamber_id(), r2->get_chamber_id(), *r2, temporary_vector_of_directions[index].first, *dx));
                r2->get_list_of_door().push_back(std::make_shared<door>(r2->get_chamber_id(), r1->get_chamber_id(), *r1, temporary_vector_of_directions[index].second, *dx));
                
                matrix_of_chamber_transitions[r1->get_chamber_id()-1][r2->get_chamber_id()-1]=true;
                matrix_of_chamber_transitions[r2->get_chamber_id()-1][r1->get_chamber_id()-1]=true;

                dx->set_index(vector_of_door_objects.size());
                vector_of_door_objects.push_back(std::move(dx));
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

void skarabeusz::generator::generate_chambers_guardians()
{
    // in fact the guardians are part of the chamber, but we need to add some
    // words they typically use
    for (auto & c: target.vector_of_chambers)
    {
        c->my_guardian->set_favourite_adjectives(_("smart"), _("foolish"));
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
    if (amount_of_chambers < max_z_range)
    {
        throw std::runtime_error("the amount of chambers must not be lower than z_range");
    }
    
    for (unsigned i=1; i<=max_z_range; i++)
    {
        unsigned x,y,z=i-1;
        do
        {
            g.get_random_room_on_level_z(x, y, z);
        }
        while (array_of_rooms[x][y][z]->get_is_assigned_to_any_chamber());
        
        array_of_rooms[x][y][z]->assign_to_chamber(i, true);
        
        vector_of_chambers[i-1]->get_seed().set_x(x);
        vector_of_chambers[i-1]->get_seed().set_y(y);
        vector_of_chambers[i-1]->get_seed().set_z(z);
        
        DEBUG("seed room for " << i << " is " << x << "," << y << "," << z);
    }
    
    
    for (unsigned i=max_z_range+1; i<=amount_of_chambers; i++)
    {
        unsigned x,y,z;
        do
        {
            g.get_random_room(x, y, z);
        }
        while (array_of_rooms[x][y][z]->get_is_assigned_to_any_chamber());
        
        array_of_rooms[x][y][z]->assign_to_chamber(i, true);
        
        vector_of_chambers[i-1]->get_seed().set_x(x);
        vector_of_chambers[i-1]->get_seed().set_y(y);
        vector_of_chambers[i-1]->get_seed().set_z(z);
        
        DEBUG("seed room for " << i << " is " << x << "," << y << "," << z);
    }    
}

void skarabeusz::maze::create_latex(const std::string & prefix)
{
    std::stringstream s;
    s << prefix << ".tex";
    
    std::ofstream file_stream(s.str());
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
    << "\\usepackage[english,russian]{babel}\n"
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
        char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
        snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("This is a map of the level %i."), z+1);
        
        file_stream
        << "\\newpage\n"
        << buffer << "\\\\\n"
        << "\\includegraphics[width=16cm,height=12cm]{" << prefix << "_" << z << ".png}\\\\\n";
    }

    file_stream << "\\newpage\n";
    
    for (unsigned z=0; z<amount_of_paragraphs; z++)
    {
        vector_of_paragraphs[z]->print_latex(file_stream);
    }
        
    file_stream
    << "\\end{document}\n";
}


void skarabeusz::hyperlink::print_html(std::ostream & s) const 
{
    s << "<a href=\"map_" << my_paragraph.get_number() << ".html\">" << my_paragraph.get_number() << "</a>\n";
}

void skarabeusz::hyperlink::print_latex(std::ostream & s) const 
{ 
    s << "\\hyperlink{par " << my_paragraph.get_number() << "}{" << my_paragraph.get_number() << "}"; 
}

void skarabeusz::maze::create_html_index(const std::string & prefix, const std::string & language, const std::string & html_head)
{
    std::ofstream file_stream("index.html");
    std::stringstream teleport_code_stream;
    
    file_stream 
        << "<!DOCTYPE html>\n" << "<html lang=\"" << convert_language_to_html_abbreviation(language) << "\">"
        << html_head
        << "<body>\n";

    if (bootstrap)
    {
        file_stream
        << "<div class=\"container-fluid\">\n"
        << "<div class=\"row\">\n"
        << "<div class=\"col-12\">\n"
        << "<div class=\"bokor-regular\" style=\"font-size:100px; text-align:center;\">Skarabeusz</div>"
        << "</div>\n"
        << "</div>\n"

        << "<div class=\"row\">\n"
        << "<div class=\"col-2\">\n"
        << "</div>\n"
        << "<div class=\"col-8\">\n";
    }

    file_stream << "<script language=\"JavaScript\">\n"
        << "function teleport(m)\n"
        << "{\n"
        << "var i=Math.floor(Math.random()*(m-1))+1;\n"
        << "window.location=\"" << prefix << "_\"+ i + \".html\";\n" 
        << "return 0;\n"
        << "}\n"
        << "</script>\n";
    
        
    teleport_code_stream << "<button onclick=\"teleport(" << amount_of_paragraphs << ")\">" << _("teleport") << "</button>";
        
        
    char buffer[SKARABEUSZ_MAX_MESSAGE_BUFFER];
    snprintf(buffer, SKARABEUSZ_MAX_MESSAGE_BUFFER-1, _("Hello! Press %s in order to enter the maze."), teleport_code_stream.str().c_str());
    file_stream << buffer;
                
    if (bootstrap)
    {
        file_stream << "</div>"
        << "<div class=\"col-2\">\n"
        << "</div>\n"
        << "</div>\n"
        << "</div>\n"
        << "<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz\" crossorigin=\"anonymous\"></script>\n";
    }

    file_stream
        << "</body>\n"
        << "</html>\n";
}

const std::string skarabeusz::maze::convert_language_to_html_abbreviation(const std::string & language) const
{
    if (language == "polish")
        return "pl";
    if (language == "finnish")
        return "fi";
    if (language == "russian")
        return "ru";
    return "en";
}


void skarabeusz::maze::create_html(const std::string & prefix, const std::string & language, const std::string & html_head_file)
{
    std::stringstream html_head_stringstream;
    std::string line;
    if (html_head_file != "")
    {
        std::ifstream head_stream(html_head_file);
        while (std::getline(head_stream, line))
        {
            html_head_stringstream << line << "\n";
        }
    }
    else
    {
        html_head_stringstream << "<head><meta charset=\"UTF-8\">";

        if (bootstrap)
        {
            html_head_stringstream <<
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"

            << "<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH\" crossorigin=\"anonymous\">\n"

            // Google fonts - Bokor

            << "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">\n"
            << "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>\n"
            << "<link href=\"https://fonts.googleapis.com/css2?family=Bokor&display=swap\" rel=\"stylesheet\">\n"

            << "<style> .bokor-regular { font-family: \"Bokor\", system-ui; font-weight: 400; font-style: normal;} </style>\n";            ;
        }

        html_head_stringstream << "</head>";
    }
    
    create_html_index(prefix, language, html_head_stringstream.str());
    
    for (unsigned z=0; z<amount_of_paragraphs; z++)
    {
        std::stringstream s;
        s << prefix << "_" << (z+1) << ".html";
        
        std::ofstream file_stream(s.str());
        file_stream 
        << "<!DOCTYPE html>\n"
        << "<html lang=\"" << convert_language_to_html_abbreviation(language) << "\">"
        << html_head_stringstream.str()
        << "<body>\n";
        
        vector_of_paragraphs[z]->print_html(file_stream);
        
        if (bootstrap)
        {
            file_stream << "<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz\" crossorigin=\"anonymous\"></script>\n";
        }

        file_stream
        << "</body>\n"
        << "</html>\n";
        
    }
}


void skarabeusz::generator::get_random_room_on_level_z(unsigned & x, unsigned & y, unsigned z)
{
    x = x_distr(gen);
    y = y_distr(gen);
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
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->get_x() < b->get_x(); });
    
    return temporary_vector[0]->get_x();
}

unsigned skarabeusz::maze::get_y1_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->get_y() < b->get_y(); });
    
    return temporary_vector[0]->get_y();
}

unsigned skarabeusz::maze::get_z1_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->get_z() < b->get_z(); });
    
    return temporary_vector[0]->get_z();
}

unsigned skarabeusz::maze::get_x2_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->get_x() > b->get_x(); });
    
    return temporary_vector[0]->get_x();

}

unsigned skarabeusz::maze::get_y2_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->get_y() > b->get_y(); });
    
    return temporary_vector[0]->get_y();
}

unsigned skarabeusz::maze::get_z2_of_chamber(unsigned id) const
{
    std::vector<std::shared_ptr<room>> temporary_vector;
    
    get_all_rooms_of_chamber(id, temporary_vector);
    
    std::sort(temporary_vector.begin(), temporary_vector.end(), [](auto &a, auto &b){ return a->get_z() > b->get_z(); });
    
    return temporary_vector[0]->get_z();
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

void skarabeusz::maze::report(std::ostream & s) const
{
    s << "amount of chambers: " << vector_of_chambers.size() << "\n";
    s << "amount of paragraphs: " << vector_of_paragraphs.size() << "\n";
    s << "amount of paragraph connections: " << vector_of_paragraph_connections.size() << "\n";
}

void skarabeusz::maze::create_maps_html(const map_parameters & mp, const std::string & prefix, const resources & r)
{
    unsigned z=0;
    const auto & figdwarf{r.get_resource_image("figure_dwarf")};
    const auto & fighero{r.get_resource_image("figure_hero")};
    const auto & compass(r.get_resource_image("compass"));
    
    for (unsigned p=0; p<amount_of_paragraphs; p++)
    {
        z = vector_of_paragraphs[p]->get_z();
        
        image i(800, 600);
        int white = i.allocate_color(255, 255, 255);
        int black = i.allocate_color(0,0,0);
        int red = i.allocate_color(255,0,0);
        int blue = i.allocate_color(100,100,255);
        int green = i.allocate_color(0,255,0);


        for (auto & c: vector_of_chambers)
        {
            int id = c->id;
            int x1 = get_x1_of_chamber(id);
            int y1 = get_y1_of_chamber(id);
            int z1 = get_z1_of_chamber(id);
            int x2 = get_x2_of_chamber(id);
            int y2 = get_y2_of_chamber(id);
            int z2 = get_z2_of_chamber(id);

            if (z1 == z && z2 == z)
            {
                i.print_string(
                            x1*mp.room_width+(x2-x1)*mp.room_width/2 + mp.room_width/2-10,
                            y1*mp.room_height+(y2-y1)*mp.room_height/2+mp.room_height/2-10,
                            c->my_name.c_str(),black);
            }
        }
        
        for (unsigned x=0; x<max_x_range; x++)
        {
            for (unsigned y=0; y<max_y_range; y++)
            {
                bool skip_filling = false;

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
                
                for (auto & d: array_of_rooms[x][y][z]->get_list_of_door())
                {
                    i.fill_ellipse((x+d->get_target_room().get_x())*mp.room_width/2+mp.room_width/2,(y+d->get_target_room().get_y())*mp.room_height/2+mp.room_height/2, mp.room_width/2,mp.room_height/2, blue);
                    i.print_string(x*mp.room_width+mp.room_width/2-10,y*mp.room_height+mp.room_height/2-10, array_of_rooms[x][y][z]->get_name(), black);
                }
                
                if (x == vector_of_paragraphs[p]->get_x() && y == vector_of_paragraphs[p]->get_y())
                {
                    //i.fill_ellipse(x*mp.room_width+mp.room_width/2,y*mp.room_height+mp.room_height/2,mp.room_width/2,mp.room_height/2, green);                    
                    i.copy(fighero.get_image(), x*mp.room_width,y*mp.room_height, mp.room_width, mp.room_height);                                                                        
                    skip_filling=true;
                }
                                
                if (array_of_rooms[x][y][z]->get_is_seed_room())
                {
                    if (!skip_filling)
                    {
                        //i.fill_ellipse(x*mp.room_width+mp.room_width/2,y*mp.room_height+mp.room_height/2,mp.room_width/2,mp.room_height/2, red);                    
                        i.copy(figdwarf.get_image(), x*mp.room_width,y*mp.room_height, mp.room_width, mp.room_height);                                                                        
                    }
                    i.print_string(x*mp.room_width+mp.room_width/2-10,y*mp.room_height+mp.room_height/2-10, array_of_rooms[x][y][z]->get_name(), black);
                    
                }
                
                
            }
        }
        
        
        FILE * pngout=nullptr;
        
        std::stringstream map_name_stream;
        
        map_name_stream << prefix << "_" << z << "_" << p << ".png";
        
        pngout = fopen(map_name_stream.str().c_str(), "wb");
        gdImagePng(i.get_image_pointer(), pngout);
        fclose(pngout);
    }


    FILE * pngout=nullptr;
    pngout = fopen("compass.png", "wb");
    gdImagePng(const_cast<image&>(compass.get_image()).get_image_pointer(), pngout);
    fclose(pngout);
}


void skarabeusz::maze::create_maps_latex(const map_parameters & mp, const std::string & prefix)
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
                    i.print_string(x*mp.room_width+mp.room_width/2-10,y*mp.room_height+mp.room_height/2-10, array_of_rooms[x][y][z]->get_name(), black);
                    
                }
                
                for (auto & d: array_of_rooms[x][y][z]->get_list_of_door())
                {
                    i.fill_ellipse((x+d->get_target_room().get_x())*mp.room_width/2+mp.room_width/2,(y+d->get_target_room().get_y())*mp.room_height/2+mp.room_height/2, mp.room_width/2,mp.room_height/2, blue);
                    i.print_string(x*mp.room_width+mp.room_width/2-10,y*mp.room_height+mp.room_height/2-10, array_of_rooms[x][y][z]->get_name(), black);
                }
                
            }
            
        }        
        
        
        FILE * pngout=nullptr;
        
        std::stringstream map_name_stream;
        
        map_name_stream << prefix << "_" << z << ".png";
        
        pngout = fopen(map_name_stream.str().c_str(), "wb");
        gdImagePng(i.get_image_pointer(), pngout);
        fclose(pngout);
    }
}

