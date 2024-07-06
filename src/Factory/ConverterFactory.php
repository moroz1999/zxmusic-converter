<?php
declare(strict_types=1);

namespace ZxMusic\Factory;

use InvalidArgumentException;
use Psr\Container\ContainerExceptionInterface;
use Psr\Container\ContainerInterface;
use Psr\Container\NotFoundExceptionInterface;
use ZxMusic\Service\Converter\ConverterInterface;
use ZxMusic\Service\Converter\ConverterType;

readonly final class ConverterFactory
{

    public function __construct(
        private ContainerInterface $container,
        private array              $converterMap,
    )
    {

    }

    /**
     * @param \ZxMusic\Service\Converter\ConverterType $type
     * @return \ZxMusic\Service\Converter\ConverterInterface
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
         * @var \ZxMusic\Service\Converter\ConverterInterface $converter
         */
        $converter = $this->container->get($this->converterMap[$typeKey]);
        return $converter;
    }
}