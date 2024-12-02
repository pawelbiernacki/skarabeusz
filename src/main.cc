#include "skarabeusz.h"
#include "config.h"
#include <libintl.h>
#include <locale.h>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <fstream>

int main(int argc, char * argv[])
{
    unsigned x_range=10,y_range=7,z_range=1,
    amount_of_chambers=5,
    max_amount_of_keys_to_hold=2,
    amount_of_alternative_endings=1;

    std::string language = "english";
    std::string prefix = "map";
    std::string html_head_filename="";
    enum class output_type { HTML, LATEX } output = output_type::LATEX;
    bool hints = false, stories = false, books = false;
    skarabeusz::generator_parameters::stories_mode_type stories_mode = skarabeusz::generator_parameters::stories_mode_type::RANDOM;
    unsigned amount_of_heroes = 1;
    skarabeusz::resources r;

    for (unsigned i=1; i<argc; i++)
    {
        if (!strcmp(argv[i], "-v"))
        {
            std::cout << "skarabeusz " << PACKAGE_VERSION << "\n";
            exit(0);
        }
        else
        if (!strcmp(argv[i], "-o"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "latex or html expected\n";
                exit(0);
            }

            if (!strcmp(argv[i], "latex"))
            {
                output = output_type::LATEX;
            }
            else
            if (!strcmp(argv[i], "html"))
            {
                output = output_type::HTML;
            }
            else
            {
                std::cerr << "unsupported output " << argv[i] << "\n";
                exit(-1);
            }
        }
        else
        if (!strcmp(argv[i], "--head-file"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "filename expected\n";
                exit(0);
            }
            html_head_filename = std::string(argv[i]);
        }
        else
        if (!strcmp(argv[i], "-x"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "value expected\n";
                exit(0);
            }
            x_range = atoi(argv[i]);
        }
        else
        if (!strcmp(argv[i], "-y"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "value expected\n";
                exit(0);
            }
            y_range = atoi(argv[i]);
        }
        else
        if (!strcmp(argv[i], "-z"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "value expected\n";
                exit(0);
            }
            z_range = atoi(argv[i]);
        }
        else
        if (!strcmp(argv[i], "-c"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "value expected\n";
                exit(0);
            }
            amount_of_chambers = atoi(argv[i]);
        }
        else
        if (!strcmp(argv[i], "-a"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "value expected\n";
                exit(0);
            }

            amount_of_alternative_endings = atoi(argv[i]);
        }
        else
        if (!strcmp(argv[i], "-p"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "prefix expected\n";
                exit(0);
            }
            prefix = std::string(argv[i]);
        }
        else
        if (!strcmp(argv[i], "-l"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "language name expected\n";
                exit(0);
            }

            language = std::string(argv[i]);
        }
        else
        if (!strcmp(argv[i], "--max-keys"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "value expected\n";
                exit(0);
            }

            max_amount_of_keys_to_hold = atoi(argv[i]);
        }
        else
        if (!strcmp(argv[i], "--hints"))
        {
            hints = true;
        }
        else
        if (!strcmp(argv[i], "--stories"))
        {
            stories = true;

            i++;
            if (i >= argc)
            {
                std::cerr << "good or bad or random or alignment expected\n";
                exit(0);
            }

            if (!strcmp(argv[i], "good"))
            {
                stories_mode = skarabeusz::generator_parameters::stories_mode_type::GOOD;
            }
            else
            if (!strcmp(argv[i], "bad"))
            {
                stories_mode = skarabeusz::generator_parameters::stories_mode_type::BAD;
            }
            else
            if (!strcmp(argv[i], "random"))
            {
                stories_mode = skarabeusz::generator_parameters::stories_mode_type::RANDOM;
            }
            else
            if (!strcmp(argv[i], "alignment"))
            {
                stories_mode = skarabeusz::generator_parameters::stories_mode_type::ALIGNMENT_SPECIFIC;
            }
            else
            {
                std::cerr << "unrecognized stories generating mode\n";
                exit(1);
            }
        }
        else
        if (!strcmp(argv[i], "--books"))
        {
            books = true;
        }
        else
        if (!strcmp(argv[i], "--heroes"))
        {
            i++;
            if (i >= argc)
            {
                std::cerr << "value expected\n";
                exit(0);
            }

            amount_of_heroes = atoi(argv[i]);
        }
        else
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            std::cout << "skarabeusz supports following options:\n";
            std::cout << "          -l (english|polish|russian|finnish) - select the output language\n";
            std::cout << "          -v                          - print the version\n";
            std::cout << "          -o (latex|html)             - select the output mode\n";
            std::cout << "          --head-file <filename>      - set the header file name\n";
            std::cout << "          -x <value>                  - width of a maze floor\n";
            std::cout << "          -y <value>                  - height of a maze floor\n";
            std::cout << "          -z <value>                  - amount of maze floors\n";
            std::cout << "          -c <value>                  - amount of chambers\n";
            std::cout << "          -a <value>                  - amount of alternative endings\n";
            std::cout << "          -p <prefix>                 - output file prefix\n";
            std::cout << "          --max-keys <value>          - max amount of keys\n";
            std::cout << "          --hints                     - hints\n";
            std::cout << "          --stories (good|bad|random|alignment) - generate stories\n";
            std::cout << "          --books                     - generate books\n";
            std::cout << "          --heroes <value>            - amount of heroes\n";
            exit(0);
        }
        else
        {
            std::cerr << "unrecognized option " << argv[i] << "\n";
            exit(1);
        }
    }

    if (amount_of_chambers < z_range)
    {
        std::cerr << "The amount of chambers must be equal or greater than the <z range>\n";
        exit(1);
    }
    if (amount_of_heroes==0 && (stories || books))
    {
        std::cerr << "I need at least one hero for the stories and books\n";
        exit(1);
    }

    if (language == "english")
    {
        setlocale(LC_ALL, "en_US.UTF-8");
    }
    else
    if (language == "polish")
    {
        setlocale(LC_ALL, "pl_PL.UTF-8");
    }
    else
    if (language == "russian")
    {
        setlocale(LC_ALL, "ru_RU.UTF-8");
    }
    else
    if (language == "finnish")
    {
        setlocale(LC_ALL, "fi_FI.UTF-8");
    }
    else
    {
        std::cerr << "unsupported language " << language << "\n";
        exit(1);
    }

    bindtextdomain ("skarabeusz", LOCALEDIR);
    textdomain("skarabeusz");


    skarabeusz::generator_parameters gp{x_range,y_range,z_range, // x,y,z
                3, // max amount of magic items (not used now!)
                amount_of_chambers, // amount of chambers
                max_amount_of_keys_to_hold,// max amount of keys to hold
                amount_of_alternative_endings,
                hints,
                stories,
                stories_mode,
                books,
                amount_of_heroes};
    skarabeusz::maze m;

    skarabeusz::generator g{gp, m};

    g.run();

    m.report(std::cout);

    skarabeusz::map_parameters mp{80,80};
    r.add_resource(std::make_unique<skarabeusz::resource_image>("figure_dwarf", "/usr/local/share/skarabeusz/figure_dwarf.png"));
    r.add_resource(std::make_unique<skarabeusz::resource_image>("figure_hero", "/usr/local/share/skarabeusz/figure_hero.png"));
    r.add_resource(std::make_unique<skarabeusz::resource_image>("compass", "/usr/local/share/skarabeusz/compass.png"));

    switch (output)
    {
        case output_type::LATEX:
            m.create_maps_latex(mp, prefix);
            m.create_latex(prefix);
            break;

        case output_type::HTML:
            m.create_maps_html(mp, prefix, r);
            m.create_html(prefix, language, html_head_filename);
            break;

        default:;
    }

    return 0;
}
