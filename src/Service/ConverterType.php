<?php
declare(strict_types=1);

namespace ZxMusic\Service;

enum ConverterType: string
{
    case ZXTUNE = 'zxtune';
    case ARKOS = 'arkos';
    case FURNACE = 'furnace';
    case CHIPNSFX = 'chipnsfx';
}
