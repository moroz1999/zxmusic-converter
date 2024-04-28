<?php
declare(strict_types=1);

namespace ZxMusic\Factory;

use InvalidArgumentException;
use Psr\Container\ContainerExceptionInterface;
use Psr\Container\ContainerInterface;
use Psr\Container\NotFoundExceptionInterface;
use ZxMusic\Service\ConverterInterface;
use ZxMusic\Service\ConverterType;

readonly final class ConverterFactory
{

    public function __construct(
        private ContainerInterface $container,
        private array              $converterMap,
    )
    {

    }

    /**
     * @param ConverterType $type
     * @return ConverterInterface
     * @throws ContainerExceptionInterface
     * @throws NotFoundExceptionInterface
     */
    public function getConverter(ConverterType $type): ConverterInterface
    {
        $typeKey = $type->value;

        if (!isset($this->converterMap[$typeKey])) {
            throw new InvalidArgumentException("Unsupported converter type: {$typeKey}");
        }
        /**
         * @var ConverterInterface $converter
         */
        $converter = $this->container->get($this->converterMap[$typeKey]);
        return $converter;
    }
}