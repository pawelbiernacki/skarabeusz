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
            html_head_filename = std::string(argv[++i]);
        }
        else
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
        if (!strcmp(argv[i], "-p"))
        {
            prefix = std::string(argv[++i]);
        }
        else
        if (!strcmp(argv[i], "-l"))
        {
            language = std::string(argv[++i]);
        }
        else
        if (!strcmp(argv[i], "--max-keys"))
        {
            max_amount_of_keys_to_hold = atoi(argv[++i]);
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
                3, // max amount of magic items
                amount_of_chambers, // amount of chambers
                max_amount_of_keys_to_hold,// max amount of keys to hold
                amount_of_alternative_endings};
    skarabeusz::maze m;

    skarabeusz::generator g{gp, m};

    g.run();

    skarabeusz::map_parameters mp{80,80};
    r.add_resource(std::make_unique<skarabeusz::resource_image>("figure_dwarf", "/usr/local/share/skarabeusz/figure_dwarf.png"));
    r.add_resource(std::make_unique<skarabeusz::resource_image>("figure_hero", "/usr/local/share/skarabeusz/figure_hero.png"));

    switch (output)
    {
        case output_type::LATEX:
            m.create_maps_latex(mp, prefix);
            m.create_latex(prefix);
            break;

        case output_type::HTML:
            m.create_maps_html(mp, prefix, r);
            m.create_html(prefix, html_head_filename);
            break;

        default:;
    }

    return 0;
}
