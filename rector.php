<?php

declare(strict_types=1);

use Rector\Config\RectorConfig;
use Rector\Set\ValueObject\SetList;
use Rector\TypeDeclaration\Rector\ClassMethod\AddVoidReturnTypeWhereNoReturnRector;
use Rector\ValueObject\PhpVersion;

return static function (RectorConfig $rectorConfig): void {
    $rectorConfig->paths([
        __DIR__ . '/public',
        __DIR__ . '/src',
    ]);

    $rectorConfig->phpVersion(PhpVersion::PHP_83);

    $rectorConfig->sets([
        SetList::PHP_80,
        SetList::PHP_81,
        SetList::PHP_83,
    ]);

    $rectorConfig->rules([
        AddVoidReturnTypeWhereNoReturnRector::class,
    ]);
};