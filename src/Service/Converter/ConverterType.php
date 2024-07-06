<?php
declare(strict_types=1);

namespace ZxMusic\Service\Converter;

enum ConverterType: string
{
    case ZXTUNE = 'zxtune';
    case ARKOS1 = 'arkos1';
    case ARKOS2 = 'arkos2';
    case FURNACE = 'furnace';
    case CHIPNSFX = 'chipnsfx';
}
