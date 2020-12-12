FROM python:3-slim

ARG USER=docker
ARG UID=1000
ARG GID=1000

RUN useradd -m ${USER} --uid=${UID} 

USER ${UID}:${GID}

WORKDIR /usr/src/app

COPY requirements.txt ./

RUN pip install --no-cache-dir -r requirements.txt

COPY shared.secrets ./

COPY gen_qr_codes.py ./

CMD [ "python", "./gen_qr_codes.py" ]
