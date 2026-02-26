FROM php:8.5-apache

ARG XDEBUG=false

RUN apt-get update && apt-get install -y \
        libzip-dev \
        unzip \
        libmp3lame0 \
    && docker-php-ext-install zip \
    && if [ "$XDEBUG" = "true" ]; then \
        pecl install xdebug \
        && docker-php-ext-enable xdebug; \
    fi \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY docker/xdebug.ini /usr/local/etc/php/conf.d/xdebug.ini
COPY docker/php.ini /usr/local/etc/php/conf.d/app.ini

RUN a2enmod rewrite headers

COPY docker/apache.conf /etc/apache2/sites-available/000-default.conf

WORKDIR /var/www/html

EXPOSE 80
