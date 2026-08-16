// Planetary Explorer
// An interactive, menu-driven console program for exploring the Sun,
// planets, dwarf planets, moons, and comets of our solar system.
//
// Built with only the C++ standard library - no external dependencies.
//
// Build:  g++ -std=c++17 -O2 -Wall -o planetary_explorer main.cpp
// Run:    ./planetary_explorer

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>

// ---------------------------------------------------------------------------
// Data model
// ---------------------------------------------------------------------------

// Sentinel values used to mark "not applicable" fields, since a struct is
// reused across very different kinds of bodies (a star, a planet, a moon,
// a comet all have different natural attributes).
static const double NA = -1.0;              // generic "not applicable" for distances/periods/etc.
static const double ROT_NA = -999999.0;      // rotation period unknown/not applicable

struct CelestialBody {
    std::string name;
    std::string category;      // "Star", "Planet", "Dwarf Planet", "Moon", "Comet"
    std::string parentBody;    // what it orbits (e.g. "Sun", "Jupiter"); empty for the Sun

    double distFromSunAU;      // average distance from the Sun, in AU (planets/dwarf planets)
    double distFromSunKm;      // average distance from the Sun, in km
    double distFromParentKm;   // average distance from parent body, in km (moons only)

    double perihelionAU;       // closest approach to the Sun, in AU (comets)
    double aphelionAU;         // farthest point from the Sun, in AU (comets)

    double orbitalPeriodDays;  // time for one full orbit, in Earth days
    double rotationHours;      // length of one rotation ("planetary day"), in hours.
                                // Negative = retrograde rotation. ROT_NA = unknown/not applicable.
    double diameterKm;         // diameter in km
    double surfaceGravityG;    // surface gravity relative to Earth (1.0 = same as Earth)

    int moonCount;             // number of known moons (-1 = not applicable)
    std::string discovery;     // discovery info
    std::vector<std::string> facts; // assorted interesting facts
};

static std::vector<CelestialBody> g_bodies;

// ---------------------------------------------------------------------------
// Formatting helpers
// ---------------------------------------------------------------------------

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

std::string commaFormat(double value) {
    long long intPart = static_cast<long long>(std::llround(value));
    bool negative = intPart < 0;
    if (negative) intPart = -intPart;
    std::string s = std::to_string(intPart);
    int insertPos = static_cast<int>(s.size()) - 3;
    while (insertPos > 0) {
        s.insert(insertPos, ",");
        insertPos -= 3;
    }
    return (negative ? "-" : "") + s;
}

std::string fmtAU(double au) {
    if (au < 0) return "N/A";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << au << " AU";
    return oss.str();
}

std::string fmtKm(double km) {
    if (km < 0) return "N/A";
    std::ostringstream oss;
    if (km >= 1'000'000.0) {
        oss << std::fixed << std::setprecision(2) << (km / 1'000'000.0) << " million km";
    } else {
        oss << commaFormat(km) << " km";
    }
    return oss.str();
}

std::string fmtOrbitalPeriod(double days) {
    if (days < 0) return "N/A (unique case -- see facts below)";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    if (days >= 365.25) {
        oss << (days / 365.25) << " Earth years (" << std::setprecision(0) << days << " days)";
    } else {
        oss << days << " Earth days";
    }
    return oss.str();
}

std::string fmtRotation(double hours) {
    if (hours <= ROT_NA / 2) return "N/A";
    bool retro = hours < 0;
    double h = std::fabs(hours);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    if (h >= 24.0) {
        oss << (h / 24.0) << " Earth days (" << std::setprecision(1) << h << " hours)";
    } else {
        oss << h << " hours";
    }
    if (retro) oss << "  [retrograde rotation -- spins backwards vs. its orbit]";
    return oss.str();
}

std::string fmtGravity(double g) {
    if (g < 0) return "N/A";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << g << " g (relative to Earth)";
    return oss.str();
}

void printRule(char c = '=', int width = 70) {
    std::cout << std::string(width, c) << "\n";
}

void pressEnterToContinue() {
    std::cout << "\nPress Enter to continue...";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

// Reads an integer choice within [lo, hi], reprompting on bad input.
int getMenuChoice(int lo, int hi, const std::string& prompt = "Enter your choice: ") {
    int choice;
    while (true) {
        std::cout << prompt;
        if (std::cin >> choice && choice >= lo && choice <= hi) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return choice;
        }
        if (std::cin.eof()) {
            std::cout << "\nInput closed. Goodbye.\n";
            std::exit(0);
        }
        std::cout << "Invalid selection. Please enter a number between "
                  << lo << " and " << hi << ".\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

std::string getLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// ---------------------------------------------------------------------------
// Data initialization
// ---------------------------------------------------------------------------

void addBody(const CelestialBody& b) {
    g_bodies.push_back(b);
}

void initData() {
    g_bodies.clear();

    // -------------------------- The Sun --------------------------
    addBody({
        "The Sun", "Star", "",
        0.0, 0.0, NA,
        NA, NA,
        NA,             // does not orbit anything in this model
        (25.05 * 24.0), // ~25.05 days at the equator (rotation varies with latitude)
        1'392'700.0,
        27.9,           // surface gravity ~27.9x Earth's
        -1,
        "Formed roughly 4.6 billion years ago",
        {
            "The Sun contains about 99.86% of the total mass of the solar system.",
            "It is a G-type main-sequence star (a 'yellow dwarf') fusing about 600 million tons of hydrogen into helium every second.",
            "Light from the Sun takes about 8 minutes and 20 seconds to reach Earth.",
            "Surface temperature is about 5,500 C (9,940 F); the core reaches roughly 15 million C.",
            "The Sun rotates faster at its equator (~25 days) than near its poles (~35 days) because it is a giant ball of plasma, not a solid body.",
            "In about 5 billion years the Sun is expected to become a red giant, then shed its outer layers to leave a white dwarf."
        }
    });

    // -------------------------- Planets --------------------------
    addBody({
        "Mercury", "Planet", "Sun",
        0.387, 57'900'000.0, NA,
        NA, NA,
        87.97,
        1407.6,   // 58.6 Earth days
        4879.0,
        0.38,
        0,
        "Known since antiquity",
        {
            "Smallest planet in the solar system and closest to the Sun.",
            "Has the most eccentric orbit of the classical planets and the most extreme temperature swings: about -180C at night to 430C during the day.",
            "A single day on Mercury (sunrise to sunrise) lasts about 176 Earth days -- two full Mercury years -- because of the interplay between its slow rotation and fast orbit.",
            "Has a very thin exosphere instead of a true atmosphere, so it cannot retain heat."
        }
    });

    addBody({
        "Venus", "Planet", "Sun",
        0.723, 108'200'000.0, NA,
        NA, NA,
        224.70,
        -5832.5,  // 243 Earth days, retrograde
        12'104.0,
        0.90,
        0,
        "Known since antiquity",
        {
            "Hottest planet in the solar system (surface ~465C) due to a runaway greenhouse effect from its thick CO2 atmosphere.",
            "Rotates backwards (retrograde) compared to most planets, and so slowly that a day on Venus (243 Earth days) is longer than its year (225 Earth days).",
            "Surface pressure is about 92 times that of Earth -- similar to being nearly 1 km underwater.",
            "Often called Earth's 'sister planet' because of similar size and mass, but conditions are utterly different."
        }
    });

    addBody({
        "Earth", "Planet", "Sun",
        1.000, 149'600'000.0, NA,
        NA, NA,
        365.25,
        23.93,
        12'742.0,
        1.00,
        1,
        "N/A -- our home",
        {
            "The only known planet with confirmed life.",
            "About 71% of the surface is covered by water.",
            "Has a single large natural satellite, the Moon, which stabilizes Earth's axial tilt and drives ocean tides.",
            "Earth's magnetic field, generated by its molten iron core, shields the surface from most harmful solar radiation."
        }
    });

    addBody({
        "Mars", "Planet", "Sun",
        1.524, 227'900'000.0, NA,
        NA, NA,
        686.98,
        24.62,
        6'779.0,
        0.38,
        2,
        "Known since antiquity",
        {
            "Home to Olympus Mons, the tallest known volcano in the solar system (~21.9 km high).",
            "Also home to Valles Marineris, a canyon system over 4,000 km long.",
            "A Martian day (called a 'sol') is only about 37 minutes longer than an Earth day.",
            "Its reddish color comes from iron oxide (rust) covering much of its surface.",
            "Its two small moons, Phobos and Deimos, are thought to be captured asteroids."
        }
    });

    addBody({
        "Jupiter", "Planet", "Sun",
        5.203, 778'500'000.0, NA,
        NA, NA,
        4332.59,
        9.93,
        139'820.0,
        2.53,
        95,
        "Known since antiquity",
        {
            "Largest planet in the solar system -- more massive than all other planets combined.",
            "The Great Red Spot is a giant storm larger than Earth that has raged for at least 350 years.",
            "Spins faster than any other planet: a Jupiter day is under 10 hours, causing a visible equatorial bulge.",
            "Has a faint ring system and a powerful magnetosphere, the largest structure in the solar system after the heliosphere.",
            "Acts as a 'cosmic vacuum cleaner,' its gravity helping deflect many comets and asteroids away from the inner solar system."
        }
    });

    addBody({
        "Saturn", "Planet", "Sun",
        9.537, 1'434'000'000.0, NA,
        NA, NA,
        10759.22,
        10.70,
        116'460.0,
        1.07,
        146,
        "Known since antiquity",
        {
            "Famous for its spectacular ring system, made mostly of ice particles with some rocky debris and dust.",
            "Least dense planet in the solar system -- it would float in water if a large enough bathtub existed.",
            "Wind speeds near the equator can exceed 1,800 km/h.",
            "Its moon Titan is the only moon in the solar system with a dense atmosphere."
        }
    });

    addBody({
        "Uranus", "Planet", "Sun",
        19.191, 2'871'000'000.0, NA,
        NA, NA,
        30688.5,
        -17.24,   // retrograde
        50'724.0,
        0.89,
        28,
        "Discovered in 1781 by William Herschel",
        {
            "Rotates on its side, with an axial tilt of about 98 degrees -- likely the result of an ancient collision.",
            "Rotation is retrograde relative to its orbit, like Venus.",
            "Coldest planetary atmosphere in the solar system, dropping to about -224C, despite not being the farthest planet from the Sun.",
            "Its blue-green color comes from methane in its atmosphere absorbing red light.",
            "First planet discovered using a telescope rather than being known since antiquity."
        }
    });

    addBody({
        "Neptune", "Planet", "Sun",
        30.069, 4'495'000'000.0, NA,
        NA, NA,
        60195.0,
        16.11,
        49'244.0,
        1.14,
        16,
        "Discovered in 1846 by Johann Galle, based on predictions by Urbain Le Verrier",
        {
            "The windiest planet in the solar system, with gusts recorded up to about 2,100 km/h.",
            "First planet located through mathematical prediction rather than direct observation.",
            "Takes about 165 Earth years to orbit the Sun -- it has only completed one full orbit since its discovery in 1846 (in 2011).",
            "Its largest moon, Triton, orbits backwards and is likely a captured Kuiper Belt object."
        }
    });

    // -------------------------- Dwarf Planets --------------------------
    addBody({
        "Pluto", "Dwarf Planet", "Sun",
        39.48, 5'906'000'000.0, NA,
        NA, NA,
        90560.0,
        -153.3,   // retrograde, 6.39 days
        2377.0,
        0.063,
        5,
        "Discovered in 1930 by Clyde Tombaugh; reclassified as a dwarf planet in 2006",
        {
            "Reclassified from 'planet' to 'dwarf planet' in 2006 after the IAU adopted a formal definition of 'planet'.",
            "Pluto and its largest moon Charon are so similar in size that they orbit a shared point (barycenter) outside Pluto itself, forming a 'double dwarf planet' system.",
            "Has a heart-shaped nitrogen-ice glacier informally named Tombaugh Regio.",
            "NASA's New Horizons probe performed the first (and so far only) close flyby in July 2015."
        }
    });

    addBody({
        "Ceres", "Dwarf Planet", "Sun",
        2.766, 413'700'000.0, NA,
        NA, NA,
        1681.6,
        9.07,
        946.0,
        0.029,
        0,
        "Discovered in 1801 by Giuseppe Piazzi",
        {
            "The largest object in the asteroid belt between Mars and Jupiter, and the only dwarf planet located in the inner solar system.",
            "Originally classified as a planet, then an asteroid, then reclassified as a dwarf planet in 2006.",
            "NASA's Dawn spacecraft found bright salt deposits and evidence of a subsurface ocean of brine."
        }
    });

    addBody({
        "Eris", "Dwarf Planet", "Sun",
        67.78, 10'152'000'000.0, NA,
        NA, NA,
        203830.0,
        25.9,
        2326.0,
        0.084,
        1,
        "Discovered in 2005 by Mike Brown and team",
        {
            "Its discovery directly triggered the 2006 IAU debate that redefined 'planet' and demoted Pluto.",
            "One of the most massive known dwarf planets, roughly similar in size to Pluto.",
            "Its single known moon, Dysnomia, is named after the Greek goddess of lawlessness (daughter of Eris)."
        }
    });

    addBody({
        "Haumea", "Dwarf Planet", "Sun",
        43.13, 6'452'000'000.0, NA,
        NA, NA,
        103410.0,
        3.915,   // extremely fast rotator
        1600.0,  // elongated, roughly 2000x1500x1000 km -- this is an approximate mean
        0.044,
        2,
        "Discovered in 2004-2005 by teams led by Mike Brown and Jose Luis Ortiz",
        {
            "Spins so fast (one rotation in under 4 hours) that it has been stretched into an elongated, egg-like shape.",
            "One of the fastest-rotating large objects known in the solar system.",
            "Has a thin ring system, discovered in 2017 -- a first for a body in the Kuiper Belt.",
            "Has two known moons, Hi'iaka and Namaka, named for daughters of the Hawaiian goddess Haumea."
        }
    });

    addBody({
        "Makemake", "Dwarf Planet", "Sun",
        45.79, 6'850'000'000.0, NA,
        NA, NA,
        111845.0,
        22.48,
        1430.0,
        0.05,
        1,
        "Discovered in 2005 by Mike Brown and team",
        {
            "Second-brightest Kuiper Belt object as seen from Earth, after Pluto.",
            "Named after the creator god of the Rapa Nui people of Easter Island.",
            "Has one known moon, informally nicknamed MK 2."
        }
    });

    // -------------------------- Moons --------------------------
    addBody({
        "The Moon (Luna)", "Moon", "Earth",
        NA, NA, 384'400.0,
        NA, NA,
        27.32,
        655.7,   // tidally locked -- same as orbital period
        3474.0,
        0.166,
        -1,
        "Known since prehistory",
        {
            "Earth's only natural satellite and the fifth largest moon in the solar system.",
            "Tidally locked to Earth, so the same face always points toward us -- the famous 'far side' is never visible from Earth.",
            "Thought to have formed from debris after a Mars-sized body (sometimes called Theia) collided with the early Earth.",
            "Its gravitational pull drives Earth's ocean tides and gradually slows Earth's rotation.",
            "Is slowly drifting away from Earth at about 3.8 cm per year."
        }
    });

    addBody({
        "Phobos", "Moon", "Mars",
        NA, NA, 9'377.0,
        NA, NA,
        0.319,
        7.66,
        22.0,
        NA,
        -1,
        "Discovered in 1877 by Asaph Hall",
        {
            "Orbits Mars faster than Mars itself rotates, so it rises in the west and sets in the east twice each Martian day.",
            "Is spiraling slowly inward and is expected to eventually break apart into a ring or crash into Mars in about 30-50 million years.",
            "Named after the Greek god personifying fear (twin of Deimos)."
        }
    });

    addBody({
        "Deimos", "Moon", "Mars",
        NA, NA, 23'460.0,
        NA, NA,
        1.263,
        30.3,
        12.0,
        NA,
        -1,
        "Discovered in 1877 by Asaph Hall",
        {
            "Smaller and more distant of Mars's two moons.",
            "Likely a captured asteroid, like Phobos.",
            "Named after the Greek god personifying dread."
        }
    });

    addBody({
        "Io", "Moon", "Jupiter",
        NA, NA, 421'700.0,
        NA, NA,
        1.769,
        42.5,
        3643.0,
        NA,
        -1,
        "Discovered in 1610 by Galileo Galilei",
        {
            "The most volcanically active body in the solar system, with hundreds of active volcanoes driven by tidal heating from Jupiter's gravity.",
            "One of the four large 'Galilean moons' discovered by Galileo in 1610.",
            "Surface is covered in sulfur compounds, giving it a mottled yellow, red, and orange appearance."
        }
    });

    addBody({
        "Europa", "Moon", "Jupiter",
        NA, NA, 671'034.0,
        NA, NA,
        3.551,
        85.2,
        3122.0,
        NA,
        -1,
        "Discovered in 1610 by Galileo Galilei",
        {
            "Believed to hide a liquid water ocean beneath its icy crust -- one of the most promising places to search for extraterrestrial life.",
            "Has one of the smoothest surfaces of any known solid body, crisscrossed by long fractures.",
            "A Galilean moon, discovered alongside Io, Ganymede, and Callisto in 1610."
        }
    });

    addBody({
        "Ganymede", "Moon", "Jupiter",
        NA, NA, 1'070'412.0,
        NA, NA,
        7.155,
        171.7,
        5268.0,
        NA,
        -1,
        "Discovered in 1610 by Galileo Galilei",
        {
            "Largest moon in the solar system -- bigger than the planet Mercury (though less massive).",
            "The only moon known to generate its own magnetic field.",
            "Believed to have a subsurface saltwater ocean, possibly layered between shells of ice."
        }
    });

    addBody({
        "Callisto", "Moon", "Jupiter",
        NA, NA, 1'882'709.0,
        NA, NA,
        16.69,
        400.5,
        4821.0,
        NA,
        -1,
        "Discovered in 1610 by Galileo Galilei",
        {
            "One of the most heavily cratered bodies in the solar system, with a surface dating back nearly 4 billion years.",
            "The outermost of the four Galilean moons.",
            "May host a subsurface ocean beneath its ancient, icy crust."
        }
    });

    addBody({
        "Titan", "Moon", "Saturn",
        NA, NA, 1'221'870.0,
        NA, NA,
        15.95,
        382.7,
        5150.0,
        NA,
        -1,
        "Discovered in 1655 by Christiaan Huygens",
        {
            "The only moon in the solar system with a dense atmosphere -- thicker than Earth's.",
            "Has stable lakes and seas of liquid methane and ethane on its surface, the only other place in the solar system with standing liquid.",
            "Second-largest moon in the solar system, bigger than the planet Mercury.",
            "The Huygens probe landed on its surface in 2005 -- the most distant landing ever achieved by a spacecraft at the time."
        }
    });

    addBody({
        "Enceladus", "Moon", "Saturn",
        NA, NA, 238'020.0,
        NA, NA,
        1.370,
        32.9,
        504.0,
        NA,
        -1,
        "Discovered in 1789 by William Herschel",
        {
            "Shoots huge geysers of water-ice from its south polar region, fed by a global subsurface ocean.",
            "One of the most reflective objects in the solar system due to its clean, icy surface.",
            "A leading candidate in the search for microbial life due to hydrothermal activity suspected on its ocean floor."
        }
    });

    addBody({
        "Mimas", "Moon", "Saturn",
        NA, NA, 185'540.0,
        NA, NA,
        0.942,
        22.6,
        396.0,
        NA,
        -1,
        "Discovered in 1789 by William Herschel",
        {
            "Nicknamed the 'Death Star moon' because its giant Herschel Crater gives it a striking resemblance to the Star Wars space station.",
            "Smallest known astronomical body confirmed to be rounded by its own gravity."
        }
    });

    addBody({
        "Rhea", "Moon", "Saturn",
        NA, NA, 527'108.0,
        NA, NA,
        4.518,
        108.4,
        1527.0,
        NA,
        -1,
        "Discovered in 1672 by Giovanni Domenico Cassini",
        {
            "Saturn's second-largest moon.",
            "May have a very thin, tenuous ring system -- if confirmed, the first ring system found around a moon."
        }
    });

    addBody({
        "Iapetus", "Moon", "Saturn",
        NA, NA, 3'560'820.0,
        NA, NA,
        79.32,
        1903.7,
        1469.0,
        NA,
        -1,
        "Discovered in 1671 by Giovanni Domenico Cassini",
        {
            "Famous for its striking two-toned coloring -- one hemisphere is very dark, the other bright icy white.",
            "Has a strange, sharp equatorial ridge that gives it a walnut-like shape."
        }
    });

    addBody({
        "Titania", "Moon", "Uranus",
        NA, NA, 436'300.0,
        NA, NA,
        8.706,
        208.9,
        1578.0,
        NA,
        -1,
        "Discovered in 1787 by William Herschel",
        {
            "Largest moon of Uranus, named after the queen of the fairies in Shakespeare's A Midsummer Night's Dream.",
            "Uranus's moons are unusually named after literary characters (Shakespeare and Alexander Pope) rather than mythological figures."
        }
    });

    addBody({
        "Oberon", "Moon", "Uranus",
        NA, NA, 583'500.0,
        NA, NA,
        13.46,
        323.1,
        1523.0,
        NA,
        -1,
        "Discovered in 1787 by William Herschel",
        {
            "Second-largest and outermost of Uranus's major moons.",
            "Heavily cratered, with some evidence of past internal geologic activity."
        }
    });

    addBody({
        "Miranda", "Moon", "Uranus",
        NA, NA, 129'900.0,
        NA, NA,
        1.413,
        33.9,
        472.0,
        NA,
        -1,
        "Discovered in 1948 by Gerard Kuiper",
        {
            "Has some of the most extreme and varied terrain in the solar system, including Verona Rupes, a cliff face roughly 20 km high.",
            "Its chaotic surface suggests it may have been shattered and re-accreted at least once in its history."
        }
    });

    addBody({
        "Triton", "Moon", "Neptune",
        NA, NA, 354'759.0,
        NA, NA,
        5.877,
        -141.0,  // retrograde
        2707.0,
        NA,
        -1,
        "Discovered in 1846 by William Lassell",
        {
            "The only large moon in the solar system with a retrograde orbit (orbiting opposite to its planet's rotation), strongly suggesting it is a captured Kuiper Belt object.",
            "Has active nitrogen-gas geysers despite surface temperatures around -235C, among the coldest measured in the solar system.",
            "Tidal forces are slowly dragging Triton inward; it may eventually be torn apart into a ring, tens of millions of years from now."
        }
    });

    addBody({
        "Nereid", "Moon", "Neptune",
        NA, NA, 5'513'400.0,
        NA, NA,
        360.1,
        ROT_NA,
        340.0,
        NA,
        -1,
        "Discovered in 1949 by Gerard Kuiper",
        {
            "Has one of the most eccentric (elongated) orbits of any known moon, ranging from about 1.4 million to 9.7 million km from Neptune.",
            "Third-largest moon of Neptune."
        }
    });

    addBody({
        "Charon", "Moon", "Pluto",
        NA, NA, 19'591.0,
        NA, NA,
        6.387,
        153.3,
        1212.0,
        NA,
        -1,
        "Discovered in 1978 by James Christy",
        {
            "About half the diameter of Pluto -- the largest moon relative to its parent body in the solar system.",
            "Pluto and Charon are mutually tidally locked, always showing the same face to each other, and orbit a shared barycenter outside Pluto itself.",
            "Some astronomers consider Pluto-Charon a 'double dwarf planet' system rather than a simple planet-moon pair."
        }
    });

    // -------------------------- Comets --------------------------
    addBody({
        "Halley's Comet", "Comet", "Sun",
        NA, NA, NA,
        0.586, 35.1,
        27759.0,  // ~76 years
        ROT_NA,
        11.0,     // nucleus diameter, approx
        NA,
        -1,
        "Documented for over 2,000 years; orbit computed by Edmond Halley in 1705",
        {
            "The most famous short-period comet, visible to the naked eye roughly every 76 years.",
            "Last appeared in 1986 (visited by multiple spacecraft, including ESA's Giotto) and is next expected around 2061.",
            "Edmond Halley correctly predicted its return using Newton's laws of motion, and it was named after him following his 1742 death."
        }
    });

    addBody({
        "Comet Hale-Bopp", "Comet", "Sun",
        NA, NA, NA,
        0.914, 370.8,
        925000.0, // ~2,533 years
        ROT_NA,
        60.0,     // one of the largest known cometary nuclei
        NA,
        -1,
        "Discovered independently in 1995 by Alan Hale and Thomas Bopp",
        {
            "One of the brightest and most widely observed comets of the 20th century, visible to the naked eye for a record 18 months.",
            "Its unusually large nucleus (up to ~60 km across) explains its exceptional brightness despite being relatively far from the Sun.",
            "Won't return to the inner solar system for roughly another 2,380 years."
        }
    });

    addBody({
        "Comet Encke", "Comet", "Sun",
        NA, NA, NA,
        0.336, 4.09,
        1204.0,   // ~3.3 years, shortest known period
        ROT_NA,
        4.8,
        NA,
        -1,
        "Recognized as periodic in 1819 by Johann Franz Encke",
        {
            "Has the shortest orbital period of any known comet -- about 3.3 years.",
            "Believed to be the parent body of the Taurid meteor shower.",
            "Has been observed on more returns than any other comet."
        }
    });

    addBody({
        "Comet Hyakutake", "Comet", "Sun",
        NA, NA, NA,
        0.230, 3410.0,
        25567500.0, // roughly 70,000 years, highly approximate
        ROT_NA,
        4.0,
        NA,
        -1,
        "Discovered in 1996 by Yuji Hyakutake",
        {
            "Passed remarkably close to Earth in March 1996 -- about 15 million km, one of the closest cometary approaches in centuries.",
            "Its tail was measured at over 570 million km long, the longest ever recorded at the time.",
            "Won't return to the inner solar system for tens of thousands of years."
        }
    });

    addBody({
        "Comet NEOWISE", "Comet", "Sun",
        NA, NA, NA,
        0.29, 715.0,
        2484000.0, // roughly 6,800 years, approximate
        ROT_NA,
        5.0,
        NA,
        -1,
        "Discovered in March 2020 by the NEOWISE space telescope",
        {
            "Became the brightest comet visible from the Northern Hemisphere since Hale-Bopp in 1997.",
            "Survived its close pass by the Sun, which destroys many small comets ('sungrazers') via heat and tidal stress.",
            "Won't be visible from Earth again for about 6,800 years."
        }
    });

    addBody({
        "Comet Shoemaker-Levy 9", "Comet", "Jupiter (formerly Sun)",
        NA, NA, NA,
        NA, NA,
        NA,    // orbit was disrupted before it could complete a normal period
        ROT_NA,
        NA,    // fragmented; no single nucleus diameter applies
        NA,
        -1,
        "Discovered in 1993 by Carolyn and Eugene Shoemaker and David Levy",
        {
            "Had been captured into orbit around Jupiter and was torn apart by Jupiter's tidal forces in 1992 into a chain of fragments.",
            "In July 1994 its fragments collided with Jupiter over several days -- the first direct observation of an extraterrestrial collision in the solar system.",
            "The impacts left dark scars in Jupiter's atmosphere, some larger than Earth, visible for months afterward.",
            "This is why its orbital data above shows 'N/A' -- its orbit around the Sun was disrupted long before this collision sequence."
        }
    });
}

// ---------------------------------------------------------------------------
// Lookup / filtering helpers
// ---------------------------------------------------------------------------

std::vector<CelestialBody*> byCategory(const std::string& category) {
    std::vector<CelestialBody*> result;
    for (auto& b : g_bodies) {
        if (b.category == category) result.push_back(&b);
    }
    return result;
}

std::vector<CelestialBody*> moonsOf(const std::string& planetName) {
    std::vector<CelestialBody*> result;
    for (auto& b : g_bodies) {
        if (b.category == "Moon" && b.parentBody == planetName) result.push_back(&b);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

void printBodyDetail(const CelestialBody& b) {
    std::cout << "\n";
    printRule('=');
    std::cout << "  " << b.name << "  (" << b.category << ")\n";
    printRule('=');

    if (b.category == "Star") {
        std::cout << "Rotation Period (equatorial): " << fmtRotation(b.rotationHours) << "\n";
        std::cout << "Diameter: " << fmtKm(b.diameterKm) << "\n";
        std::cout << "Surface Gravity: " << fmtGravity(b.surfaceGravityG) << "\n";
        std::cout << "Formation: " << b.discovery << "\n";
    } else if (b.category == "Planet" || b.category == "Dwarf Planet") {
        std::cout << "Orbits: " << b.parentBody << "\n";
        std::cout << "Distance from Sun (average): " << fmtAU(b.distFromSunAU)
                  << "  (" << fmtKm(b.distFromSunKm) << ")\n";
        std::cout << "Orbital Period (year length): " << fmtOrbitalPeriod(b.orbitalPeriodDays) << "\n";
        std::cout << "Planetary Day (rotation period): " << fmtRotation(b.rotationHours) << "\n";
        std::cout << "Diameter: " << fmtKm(b.diameterKm) << "\n";
        std::cout << "Surface Gravity: " << fmtGravity(b.surfaceGravityG) << "\n";
        if (b.moonCount >= 0) std::cout << "Known Moons: " << b.moonCount << "\n";
        std::cout << "Discovery: " << b.discovery << "\n";
    } else if (b.category == "Moon") {
        std::cout << "Orbits: " << b.parentBody << "\n";
        std::cout << "Distance from " << b.parentBody << " (average): " << fmtKm(b.distFromParentKm) << "\n";
        std::cout << "Orbital Period (around " << b.parentBody << "): " << fmtOrbitalPeriod(b.orbitalPeriodDays) << "\n";
        std::cout << "Rotation Period: " << fmtRotation(b.rotationHours);
        if (b.rotationHours > ROT_NA / 2 &&
            std::fabs(std::fabs(b.rotationHours) / 24.0 - b.orbitalPeriodDays) < 0.5) {
            std::cout << "  (tidally locked -- same face always points toward " << b.parentBody << ")";
        }
        std::cout << "\n";
        std::cout << "Diameter: " << fmtKm(b.diameterKm) << "\n";
        std::cout << "Discovery: " << b.discovery << "\n";
    } else if (b.category == "Comet") {
        std::cout << "Orbits: " << b.parentBody << "\n";
        std::cout << "Perihelion (closest approach to Sun): " << fmtAU(b.perihelionAU) << "\n";
        std::cout << "Aphelion (farthest point from Sun): " << fmtAU(b.aphelionAU) << "\n";
        std::cout << "Orbital Period: " << fmtOrbitalPeriod(b.orbitalPeriodDays) << "\n";
        std::cout << "Nucleus Diameter (approx.): " << fmtKm(b.diameterKm) << "\n";
        std::cout << "Discovery: " << b.discovery << "\n";
    }

    if (!b.facts.empty()) {
        std::cout << "\nInteresting Facts:\n";
        for (auto& f : b.facts) std::cout << "  - " << f << "\n";
    }
    printRule('-');
}

void printBodySummaryLine(int index, const CelestialBody& b) {
    std::cout << "  " << std::setw(2) << index << ") " << b.name;
    if (!b.parentBody.empty() && b.category == "Moon") {
        std::cout << "  (orbits " << b.parentBody << ")";
    }
    std::cout << "\n";
}

// Shows a list, lets the user pick one to view in detail (0 to go back).
void browseList(std::vector<CelestialBody*> list, const std::string& title) {
    if (list.empty()) {
        std::cout << "\nNo entries found for " << title << ".\n";
        pressEnterToContinue();
        return;
    }
    while (true) {
        std::cout << "\n";
        printRule('=');
        std::cout << "  " << title << "\n";
        printRule('=');
        for (size_t i = 0; i < list.size(); ++i) {
            printBodySummaryLine(static_cast<int>(i) + 1, *list[i]);
        }
        std::cout << "   0) Back\n";
        int choice = getMenuChoice(0, static_cast<int>(list.size()));
        if (choice == 0) return;
        printBodyDetail(*list[choice - 1]);

        // If this is a planet or dwarf planet with moons, offer to view them.
        CelestialBody& chosen = *list[choice - 1];
        if (chosen.category == "Planet" || chosen.category == "Dwarf Planet") {
            auto moons = moonsOf(chosen.name);
            if (!moons.empty()) {
                std::cout << "\n" << chosen.name << " has " << moons.size()
                          << " notable moon(s) listed in this program. View them? (y/n): ";
                std::string ans;
                std::getline(std::cin, ans);
                if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
                    browseList(moons, "Moons of " + chosen.name);
                    continue;
                }
            }
        }
        pressEnterToContinue();
    }
}

// ---------------------------------------------------------------------------
// Feature: search
// ---------------------------------------------------------------------------

void searchByName() {
    std::string query = toLower(getLine("\nEnter a search term (name or partial name): "));
    if (query.empty()) return;

    std::vector<CelestialBody*> matches;
    for (auto& b : g_bodies) {
        if (toLower(b.name).find(query) != std::string::npos) matches.push_back(&b);
    }

    if (matches.empty()) {
        std::cout << "No bodies found matching \"" << query << "\".\n";
        pressEnterToContinue();
        return;
    }
    browseList(matches, "Search Results for \"" + query + "\"");
}

// ---------------------------------------------------------------------------
// Feature: compare two bodies
// ---------------------------------------------------------------------------

CelestialBody* findByExactOrPartialName(const std::string& query) {
    std::string q = toLower(query);
    // Prefer exact match first.
    for (auto& b : g_bodies) {
        if (toLower(b.name) == q) return &b;
    }
    for (auto& b : g_bodies) {
        if (toLower(b.name).find(q) != std::string::npos) return &b;
    }
    return nullptr;
}

std::string orbitDistanceLabel(const CelestialBody& b) {
    if (b.category == "Moon") return fmtKm(b.distFromParentKm) + " from " + b.parentBody;
    if (b.category == "Comet") {
        return "perihelion " + fmtAU(b.perihelionAU) + " / aphelion " + fmtAU(b.aphelionAU);
    }
    if (b.category == "Star") return "N/A";
    return fmtAU(b.distFromSunAU) + " (" + fmtKm(b.distFromSunKm) + ")";
}

void compareTwoBodies() {
    std::cout << "\nCompare two bodies by name (e.g. \"Earth\", \"Titan\", \"Halley's Comet\").\n";
    std::string n1 = getLine("First body: ");
    std::string n2 = getLine("Second body: ");

    CelestialBody* b1 = findByExactOrPartialName(n1);
    CelestialBody* b2 = findByExactOrPartialName(n2);

    if (!b1 || !b2) {
        std::cout << "Could not find one or both bodies. Try Search (option 6) to check spelling.\n";
        pressEnterToContinue();
        return;
    }

    std::cout << "\n";
    printRule('=');
    std::cout << std::left << std::setw(26) << " " << std::setw(22) << b1->name
              << std::setw(22) << b2->name << "\n";
    printRule('=');

    auto row = [](const std::string& label, const std::string& v1, const std::string& v2) {
        std::cout << std::left << std::setw(26) << label
                  << std::setw(22) << v1 << std::setw(22) << v2 << "\n";
    };

    row("Category:", b1->category, b2->category);
    row("Orbits:", b1->parentBody, b2->parentBody);
    row("Distance:", orbitDistanceLabel(*b1), orbitDistanceLabel(*b2));
    row("Orbital Period:", fmtOrbitalPeriod(b1->orbitalPeriodDays), fmtOrbitalPeriod(b2->orbitalPeriodDays));
    row("Rotation ('Day'):", fmtRotation(b1->rotationHours), fmtRotation(b2->rotationHours));
    row("Diameter:", fmtKm(b1->diameterKm), fmtKm(b2->diameterKm));
    printRule('=');
    pressEnterToContinue();
}

// ---------------------------------------------------------------------------
// Feature: random fact
// ---------------------------------------------------------------------------

void showRandomFact() {
    std::vector<std::pair<std::string, std::string>> allFacts; // (body name, fact)
    for (auto& b : g_bodies) {
        for (auto& f : b.facts) allFacts.push_back({b.name, f});
    }
    if (allFacts.empty()) return;
    int idx = std::rand() % static_cast<int>(allFacts.size());
    std::cout << "\n";
    printRule('*');
    std::cout << "Did you know? (" << allFacts[idx].first << ")\n";
    std::cout << allFacts[idx].second << "\n";
    printRule('*');
    pressEnterToContinue();
}

// ---------------------------------------------------------------------------
// Menus
// ---------------------------------------------------------------------------

void showMainMenu() {
    std::cout << "\n";
    printRule('=');
    std::cout << "           PLANETARY EXPLORER -- Our Solar System\n";
    printRule('=');
    std::cout <<
        "  1) The Sun\n"
        "  2) Planets\n"
        "  3) Dwarf Planets\n"
        "  4) Moons\n"
        "  5) Comets\n"
        "  6) Search by Name\n"
        "  7) Compare Two Bodies\n"
        "  8) Random Fact\n"
        "  9) About This Program\n"
        "  0) Exit\n";
    printRule('-');
}

void moonsMenu() {
    while (true) {
        std::cout << "\n";
        printRule('=');
        std::cout << "  MOONS\n";
        printRule('=');
        std::cout <<
            "  1) Browse all listed moons\n"
            "  2) Browse moons of a specific planet\n"
            "  0) Back\n";
        int choice = getMenuChoice(0, 2);
        if (choice == 0) return;
        if (choice == 1) {
            browseList(byCategory("Moon"), "All Moons");
        } else {
            auto planets = byCategory("Planet");
            auto dwarfs = byCategory("Dwarf Planet");
            std::vector<CelestialBody*> parents;
            parents.insert(parents.end(), planets.begin(), planets.end());
            parents.insert(parents.end(), dwarfs.begin(), dwarfs.end());

            std::cout << "\nWhich planet or dwarf planet?\n";
            for (size_t i = 0; i < parents.size(); ++i) {
                std::cout << "  " << (i + 1) << ") " << parents[i]->name << "\n";
            }
            std::cout << "  0) Back\n";
            int pchoice = getMenuChoice(0, static_cast<int>(parents.size()));
            if (pchoice == 0) continue;
            CelestialBody* parent = parents[pchoice - 1];
            browseList(moonsOf(parent->name), "Moons of " + parent->name);
        }
    }
}

void aboutProgram() {
    std::cout << "\n";
    printRule('=');
    std::cout << "  ABOUT PLANETARY EXPLORER\n";
    printRule('=');
    std::cout <<
        "  A self-contained C++ console program for exploring our solar\n"
        "  system: the Sun, the eight planets, five well-known dwarf\n"
        "  planets, a selection of notable moons, and several famous\n"
        "  comets. Data (distances, orbital periods, rotation periods,\n"
        "  sizes, and fun facts) is stored directly in the program using\n"
        "  only the C++ standard library -- no internet connection or\n"
        "  external files required.\n\n"
        "  Notes on the data:\n"
        "  - Distances and periods are long-term averages; real orbits\n"
        "    are elliptical, so actual values vary somewhat over time.\n"
        "  - Only a representative selection of moons is included for\n"
        "    each planet; total known-moon counts (which change as new\n"
        "    moons are discovered) are shown on each planet's page.\n"
        "  - A negative rotation period denotes retrograde rotation --\n"
        "    the body spins opposite to the direction it orbits.\n";
    printRule('-');
    pressEnterToContinue();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    initData();

    std::cout << "Welcome to Planetary Explorer!\n";

    bool running = true;
    while (running) {
        showMainMenu();
        int choice = getMenuChoice(0, 9);
        switch (choice) {
            case 1:
                printBodyDetail(g_bodies[0]); // The Sun is always first
                pressEnterToContinue();
                break;
            case 2:
                browseList(byCategory("Planet"), "Planets");
                break;
            case 3:
                browseList(byCategory("Dwarf Planet"), "Dwarf Planets");
                break;
            case 4:
                moonsMenu();
                break;
            case 5:
                browseList(byCategory("Comet"), "Comets");
                break;
            case 6:
                searchByName();
                break;
            case 7:
                compareTwoBodies();
                break;
            case 8:
                showRandomFact();
                break;
            case 9:
                aboutProgram();
                break;
            case 0:
                running = false;
                break;
        }
    }

    std::cout << "\nClear skies! Goodbye.\n";
    return 0;
}
