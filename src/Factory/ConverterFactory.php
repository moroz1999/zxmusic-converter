<?php
declare(strict_types=1);

namespace ZxMusic\Factory;

use Psr\Container\ContainerExceptionInterface;
use Psr\Container\ContainerInterface;
use Psr\Container\NotFoundExceptionInterface;
use ZxMusic\Converter\ConverterInterface;

class ConverterFactory
{

    public function __construct(
        private ContainerInterface $container,
        private array              $converterMap,
    )
    {

    }

    /**
     * @param string $type
     * @return ConverterInterface
     * @throws ContainerExceptionInterface
     * @throws NotFoundExceptionInterface
     */
    public function getConverter(string $type): ConverterInterface
    {
        if (!isset($this->converterMap[$type])) {
            throw new \InvalidArgumentException("Unsupported converter type: {$type}");
        }
        return $this->container->get($this->converterMap[$type]);
    }
}