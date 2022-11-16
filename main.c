#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"
#include <assert.h>

typedef SDL_bool bool;

#define true SDL_TRUE
#define false SDL_FALSE

typedef struct Str
{
    size_t length, size;
    char  *data;
} Str;

size_t CStrLength (const char *cstr)
{
    size_t length = 0;

    while (*cstr++)
        length++;

    return length;
}

Str StrBuild (size_t length)
{
    Str str = {
        .length = length,
        .size   = length,
        NULL,
    };

    if (length > 0)
        str.data = malloc (sizeof (char) * length);

    return str;
}

Str StrCopy (size_t length, Str toCopy)
{
    assert (length >= toCopy.length);

    Str str = {
        .length = length,
        .size   = length,
        NULL,
    };

    if (length > 0)
    {
        str.data = malloc (sizeof (char) * length);
        memcpy (str.data, toCopy.data, sizeof (char) * toCopy.length);
    }

    return str;
}

Str StrCCopy (size_t length, const char *toCopy)
{
    assert (length > 0);

    Str str = {
        .length = length,
        .size   = length,
        .data   = malloc (sizeof (char) * length),
    };

    memcpy (str.data, toCopy, sizeof (char) * length);

    return str;
}

Str CStr (const char *cstr)
{
    Str str = StrBuild (CStrLength (cstr));

    memcpy (str.data, cstr, sizeof (char) * str.length);

    return str;
}

Str StrConcatCStr (Str str, const char *cstr)
{
    size_t cLength = CStrLength (cstr), length = cLength + str.length;

    Str result = StrCopy (length, str);

    memcpy (result.data + str.length, cstr, sizeof (char) * cLength);

    free (str.data);

    return result;
}

Str StrConcatChar (Str str, const char c)
{
    Str result = StrCopy (str.length + 1, str);

    result.data[result.length - 1] = c;

    free (str.data);

    return result;
}

SDL_Texture *Cursor (SDL_Renderer *renderer, TTF_Font *font)
{
    const char *str = "█";

    SDL_Surface *s = TTF_RenderUTF8_Blended (
        font, str, (SDL_Color) { 255, 255, 255, 255 });

    assert (s != NULL);

    SDL_Texture *texture = SDL_CreateTextureFromSurface (renderer, s);

    assert (texture != NULL);

    SDL_FreeSurface (s);

    SDL_SetTextureColorMod (texture, 0, 255, 0);

    return texture;
}

SDL_Texture *cursor = NULL;

SDL_Texture *Glyph (char c, TTF_Font *font, SDL_Renderer *renderer)
{
    static SDL_Texture *textures[1000] = { NULL };

    if (c < 0)
        return cursor;

    if (textures[c])
        return textures[c];

    char str[2] = { c, '\0' };

    if (c == 9)
        str[0] = ' ';

    SDL_Surface *s = TTF_RenderText_Blended (
        font, str, (SDL_Color) { 255, 255, 255, 255 });

    if (s == NULL)
        return cursor;

    SDL_Texture *texture = SDL_CreateTextureFromSurface (renderer, s);

    assert (texture != NULL);

    SDL_FreeSurface (s);

    // NOTE: Coloring for some characters
    if (c == '(' || c == ')')
        SDL_SetTextureColorMod (texture, 255, 0, 255);

    if (c == '{' || c == '}')
        SDL_SetTextureColorMod (texture, 0, 255, 255);

    if (c == '[' || c == ']')
        SDL_SetTextureColorMod (texture, 255, 255, 0);

    if (c == ';')
        SDL_SetTextureColorMod (texture, 0, 255, 0);

    if (c == '=')
        SDL_SetTextureColorMod (texture, 255, 0, 0);

    if (c == ',')
        SDL_SetTextureColorMod (texture, 128, 64, 0);

    if (c == '*')
        SDL_SetTextureColorMod (texture, 64, 64, 255);

    if (c == '+')
        SDL_SetTextureColorMod (texture, 64, 128, 128);

    textures[c] = texture;

    return texture;
}

typedef struct Buffer
{
    size_t until;
    Str    line;
} Buffer;

Str FileRead (const char *filename)
{
    SDL_RWops *rw = SDL_RWFromFile (filename, "rb");

    assert (rw != NULL);

    Sint64 res_size = SDL_RWsize (rw);
    Str    res      = StrBuild (res_size);

    Sint64 nb_read_total = 0, nb_read = 1;
    char  *buf = res.data;

    while (nb_read_total < res_size && nb_read != 0)
    {
        nb_read = SDL_RWread (rw, buf, 1, (res_size - nb_read_total));
        nb_read_total += nb_read;
        buf += nb_read;
    }

    SDL_RWclose (rw);

    assert (nb_read_total == res_size);

    return res;
}

typedef struct Config
{
    bool valid;

    int  x, y, w, h;
    Str  file;
    bool fullScreen;
    int  logicalW, logicalH;
    Str  font;
    int  fontSize;

    Str sfx;
} Config;

bool StrCCompare (Str str, const char *cstr)
{
    size_t i = 0;

    for (i = 0; i < str.length && *cstr; i++, cstr++)
    {
        if (str.data[i] != *cstr)
            return false;
    }

    if (i != str.length)
        return false;
    else
        return true;
}

int StrToInt (Str s)
{
    int result = 0;

    for (size_t i = 0; i < s.length; i++)
    {
        result += (s.data[i] - '0') * SDL_pow (10, (s.length - 1) - i);
    }

    return result;
}

void PrintConfig (Config c)
{
    printf ("x: %d -- y: %d -- w: %d -- h: %d\n", c.x, c.y, c.w, c.h);

    printf ("file: %.*s -- fullScreen: %d -- logicalW: %d -- logicalH: %d\n",
            (int)c.file.length, c.file.data, c.fullScreen, c.logicalW,
            c.logicalH);

    printf ("font: %.*s -- fontSize: %d\n", (int)c.font.length, c.font.data,
            c.fontSize);
}

Config EmptyConfig ()
{
    return (Config) {
        false, -1,
        -1,    -1,
        -1,    { 0, 0, NULL },
        true,  -1,
        -1,    { 0, 0, NULL },
        -1,    { 0, 0, NULL },
    };
}

bool ValidateConfig (Config c)
{
    if (c.x >= 0 && c.y >= 0 && c.w > 0 && c.h > 0 && c.file.data && c.font.data
        && c.logicalW > 0 && c.logicalH > 0 && c.fontSize > 0)
        return true;
    else
        return false;
}

Config LoadConfig ()
{
    SDL_Delay (5000);

    Config result = EmptyConfig ();

    Str    config = FileRead ("config"), field, val;
    bool   active = false;
    size_t at = 0, chars = 0;

    for (size_t i = 0; i < config.length; i++)
    {
        if (config.data[i] == ':')
        {
            field = StrCCopy (chars, config.data + at);
            at    = i + 1;
            chars = 0;
        }
        else if (config.data[i] == '\n' || i == config.length - 1)
        {
            val = StrCCopy (chars, config.data + at);

            if (StrCCompare (field, "x"))
                result.x = StrToInt (val);
            else if (StrCCompare (field, "y"))
                result.y = StrToInt (val);
            else if (StrCCompare (field, "w"))
                result.w = StrToInt (val);
            else if (StrCCompare (field, "h"))
                result.h = StrToInt (val);
            else if (StrCCompare (field, "file"))
                result.file = StrCopy (val.length, val);
            else if (StrCCompare (field, "fullScreen"))
                result.fullScreen = StrToInt (val);
            else if (StrCCompare (field, "logicalW"))
                result.logicalW = StrToInt (val);
            else if (StrCCompare (field, "logicalH"))
                result.logicalH = StrToInt (val);
            else if (StrCCompare (field, "font"))
                result.font = StrCopy (val.length, val);
            else if (StrCCompare (field, "fontSize"))
                result.fontSize = StrToInt (val);
            else if (StrCCompare (field, "sfx"))
                result.sfx = StrCopy (val.length, val);

            free (val.data);
            free (field.data);

            chars = 0;

            at = i + 1;
        }
        else
        {
            chars++;
        }
    }

    return result;
}

// NOTE: Only use as tmp strings
const char *StrToCStr (Str str)
{
    static char *cstr = NULL;

    if (cstr)
        free (cstr);

    cstr = malloc (sizeof (char) * (str.length + 1));
    memcpy (cstr, str.data, sizeof (char) * str.length);
    cstr[str.length] = '\0';

    return cstr;
}

int main (int argc, char **argv)
{
    Config c = LoadConfig ();

    assert (ValidateConfig (c) == true);

    PrintConfig (c);

    Str s = FileRead (StrToCStr (c.file));

    size_t currLine = 0, chars = 0, at = 0;
    Buffer lines[10000];

    for (size_t i = 0; i < s.length; i++)
    {
        if (s.data[i] == '\n')
        {
            if (chars > 0)
            {
                lines[currLine++]
                    = (Buffer) { 0, StrCCopy (chars, s.data + at) };

                chars = 0;
            }
            else
            {
                lines[currLine++]
                    = (Buffer) { 0, { .length = 0, .size = 0, .data = NULL } };
            }

            at = i + 1;
        }
        else
        {
            chars++;
        }
    }

    bool run = true;

    SDL_Init (SDL_INIT_EVERYTHING);
    TTF_Init ();

    if (c.sfx.data)
    {
        int flags = MIX_INIT_MP3, initted = Mix_Init (flags);

        if ((initted & flags) != flags)
        {
            printf ("Mix_Init: Failed to init required ogg and mod support!\n");
            printf ("Mix_Init: %s\n", Mix_GetError ());
        }

        if (Mix_OpenAudio (44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1)
        {
            printf ("Mix_OpenAudio: %s\n", Mix_GetError ());
            exit (2);
        }

        Mix_VolumeMusic (MIX_MAX_VOLUME * .1);

        Mix_Volume (0, MIX_MAX_VOLUME * .5f);
        Mix_Volume (1, MIX_MAX_VOLUME * .5f);
        Mix_Volume (2, MIX_MAX_VOLUME * .5f);

        Mix_Chunk *typing = Mix_LoadWAV (StrToCStr (c.sfx));

        Mix_PlayChannel (-1, typing, -1);
    }

    SDL_WindowFlags wFlags = SDL_WINDOW_SHOWN;

    if (c.fullScreen)
        wFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    SDL_Window *window
        = SDL_CreateWindow ("cmacte", c.x, c.y, c.w, c.h, wFlags);

    SDL_Renderer *renderer
        = SDL_CreateRenderer (window, -1, SDL_RENDERER_ACCELERATED);

    assert (renderer != NULL);

    TTF_Font *font = TTF_OpenFont (StrToCStr (c.font), c.fontSize);

    int w, h;

    TTF_SizeText (font, "a", &w, &h);

    SDL_RenderSetLogicalSize (renderer, c.logicalW, c.logicalH);

    Uint32 curr   = SDL_GetTicks ();
    size_t length = 0, y = 0;

    SDL_SetWindowOpacity (window, .9);

    cursor = Cursor (renderer, font);

    while (run)
    {
        SDL_Event e;

        while (SDL_PollEvent (&e))
        {
            switch (e.type)
            {
                case SDL_QUIT: run = false; break;
                case SDL_TEXTINPUT: s = StrConcatCStr (s, e.text.text); break;
            }
        }

        SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);
        SDL_RenderClear (renderer);

        SDL_Rect r = { 0, 0, w, h };

        if (SDL_GetTicks () - curr > 30 && length < currLine)
        {
            Buffer *b = &lines[length];

            if (++b->until >= b->line.length)
            {
                length++;

                if ((length - y) * h > c.logicalH - h)
                    y += 10;
            }

            curr = SDL_GetTicks ();
        }

        for (size_t j = y; j <= length && j < currLine; j++)
        {
            Buffer *b = &lines[j];

            for (size_t i = 0; i < b->until && i < b->line.length; i++)
            {
                SDL_RenderCopy (renderer,
                                Glyph (b->line.data[i], font, renderer), NULL,
                                &r);

                r.x += w;
            }

            r.x = 0;
            r.y += h;
        }

        r.x = lines[length].until * w;
        r.y = (length - y) * h;

        SDL_RenderCopy (renderer, cursor, NULL, &r);
        SDL_RenderPresent (renderer);

        if (length >= currLine && c.sfx.data)
            Mix_HaltChannel (-1);
    }

    return 0;
}
